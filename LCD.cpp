#include "mbed.h"
#include "SLCD.h"
#include "keypad.h"

// rows: adjust to your tested correct order
#define R0 PTC8
#define R1 PTA5
#define R2 PTA4
#define R3 PTA12

// cols
#define C0 PTD3
#define C1 PTA2
#define C2 PTA1

int main()
{
    SLCD lcd;
    Keypad keypad(R0, R1, R2, R3, C0, C1, C2);

    char key;
    char display_buf[5] = {' ', ' ', ' ', ' ', '\0'};
    int pos = 0;

    lcd.clear();
    lcd.Home();
    lcd.printf("    ");

    printf("Keypad LCD test started\n");

    while (true) {
        key = keypad.ReadKey();

        if (key != NO_KEY) {

            // # = clear screen
            if (key == '*') {
                display_buf[0] = ' ';
                display_buf[1] = ' ';
                display_buf[2] = ' ';
                display_buf[3] = ' ';
                pos = 0;

                lcd.clear();
                lcd.Home();
                lcd.printf("%s", display_buf);

                printf("Clear\n");
            }

            // * = confirm
            else if (key == '#') {
                printf("Confirm: %s\n", display_buf);

                lcd.clear();
                lcd.Home();
                lcd.printf("DONE");

                ThisThread::sleep_for(1000ms);

                lcd.clear();
                lcd.Home();
                lcd.printf("%s", display_buf);
            }

            // normal key input
            else {
                // if already full, clear first and keep only the new key
                if (pos >= 4) {
                    display_buf[0] = ' ';
                    display_buf[1] = ' ';
                    display_buf[2] = ' ';
                    display_buf[3] = ' ';
                    pos = 0;
                }

                display_buf[pos] = key;
                pos++;

                lcd.clear();
                lcd.Home();
                lcd.printf("%s", display_buf);

                printf("Key pressed: %c\n", key);
            }
        }

        ThisThread::sleep_for(20ms);
    }
}
