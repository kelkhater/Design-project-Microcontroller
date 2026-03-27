#include "mbed.h"
#include <cstdint>
#include <stdio.h>
#include <iostream>
#include <cstring>
#include <FlashIAP.h> //Flash API from ARM is how we will write to permanent memory 
// main() runs in its own thread in the OS

typedef struct{
    int id;
    int code[4];
    uint32_t valid;
}StoredUser;

typedef struct{
    StoredUser admin;
    StoredUser users[10];
    uint32_t userCount;
    uint32_t nextID;
    uint32_t dataValid;
}MemoryWriter;

uint32_t getAddress(FlashIAP& flash){

    uint32_t flashStart = flash.get_flash_start();//address where flash mmeory begins
    uint32_t SectorSize = flash.get_flash_size();//how many bytes of Flash exist
    uint32_t flashEnd = flashStart + SectorSize;//This is the address of the end of the Flash zone

    uint32_t lastSectorSize = flash.get_sector_size(flashEnd - 1);//Returns the size of the last sector in the flash
    uint32_t userPasswordAddress = flashEnd - lastSectorSize;//This is the address of the beginning of the last sector in flash memory

    return userPasswordAddress;
}
// General flow of changing data in Flash: read-modify-erase-write
//I will make a seperate file for managing flash memory in a seperate file and import it.
int Reading(MemoryWriter* CurrentMem){
    FlashIAP flash;
    flash.init();
    uint32_t addr = getAddress(flash);
    int result = flash.read(CurrentMem, addr, sizeof(MemoryWriter));
    flash.deinit();
    return result;
}

int Writing(MemoryWriter* CurrentMem){
    if(CurrentMem == nullptr){return -1;}

    FlashIAP flash;
    int status = flash.init();
    if (status != 0){
        return status;
    }

    uint32_t addr = getAddress(flash);
    uint32_t sectorSize = flash.get_sector_size(addr);
    uint32_t pageSize = flash.get_page_size();

    uint32_t dataSize = sizeof(MemoryWriter);
    uint32_t paddedSize = ((dataSize + pageSize - 1) / pageSize) * pageSize;

    if (paddedSize > sectorSize){
        flash.deinit();
        return -3;
    }

    constexpr uint32_t FLASH_BUFFER_SIZE = 512;

    if (paddedSize > FLASH_BUFFER_SIZE){
        flash.deinit();
        return -4;
    }

    uint8_t buffer[FLASH_BUFFER_SIZE];
    memset(buffer, flash.get_erase_value(), paddedSize);
    memcpy(buffer, CurrentMem, sizeof(MemoryWriter));

    int valid = flash.erase(addr, sectorSize);
    if (valid != 0){
        flash.deinit();
        return valid;
    }

    int write = flash.program(buffer, addr, paddedSize);

    flash.deinit();
    return write;
}


void InitializeMemory(MemoryWriter* mem){
    if (mem == nullptr) return;

    mem->userCount = 0;
    mem->dataValid = 1;
    mem->nextID = 0;
    mem->admin.id = 0;
    for (int i = 0; i < 4; i++){
        mem->admin.code[i] = 0;
    }
    mem->admin.valid = 1;

    for (int u = 0; u < 10; u++){
        mem->users[u].id = 0;
        mem->users[u].valid = 0;
        for (int i = 0; i < 4; i++){
            mem->users[u].code[i] = 0;
        }
    }
}


int AddUser(MemoryWriter* currentMem, int code[4]){
    if (currentMem == nullptr) return -1;

    if (currentMem->userCount >= 10){
        return -2;
    }
    
    StoredUser newUser;
    newUser.id = currentMem -> nextID;
    newUser.valid = 1;
    for(int i = 0; i < 4; i++){
        newUser.code[i] = code[i];
    }
    currentMem -> users[currentMem -> userCount] = newUser;
    currentMem -> userCount++;
    currentMem -> nextID++;

    return 0;

}

int removeUser(MemoryWriter* currentMem, int Id){
    if(currentMem == NULL){return -1;}
        int index = -1;
    for (uint32_t i = 0; i < currentMem->userCount; i++){
        if (currentMem->users[i].id == Id && currentMem->users[i].valid == 1){
            index = i;
            break;
        }
    }
    if (index == -1) return -2;

    for (uint32_t i = index; i < currentMem->userCount - 1; i++){
        currentMem->users[i] = currentMem->users[i + 1];
    }

    currentMem->userCount--;

    currentMem->users[currentMem->userCount].id = 0;
    currentMem->users[currentMem->userCount].valid = 0;
    for (int i = 0; i < 4; i++){
        currentMem->users[currentMem->userCount].code[i] = 0;
    }

    return 0;
}


int changeAdminCode(MemoryWriter* currentMem, int newcode[4]){
    if(currentMem == NULL){return -1;}
    for(int i = 0; i < 4; i++){
        currentMem -> admin.code[i] = newcode[i];
    }
    return 0;
}

int changeUserCode(MemoryWriter* currentMem, int newCode[4], int Id){
    if(currentMem == NULL){return -1;}
    int index = -1;

    for(int i = 0; i < currentMem -> userCount; i++){
        if (currentMem -> users[i].id == Id){
            index = i;
            break;
        }
    }
    if (index == -1){
        return -2; // user not found
    }

    for(int i = 0; i < 4; i++){
        currentMem -> users[index].code[i] = newCode[i];
    }
    return 0;

}

int main(){ 
    MemoryWriter mem;
    Reading(&mem);

    if (mem.dataValid != 1){
        InitializeMemory(&mem);
        Writing(&mem);
    }  
    while(true){
        return 0;
  } 
}

