#include "mbed.h"
#include "SLCD.h"
#include "keypad.h"

// Rows
#define R0 PTC8
#define R1 PTA5
#define R2 PTA4
#define R3 PTA12

// Columns
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

    const char password[5] = {'1', '2', '3', '4', '\0'};
    bool password_mode = false;

    // pending_special is * || #
    char pending_special = NO_KEY;
    auto pending_time = Kernel::Clock::now();
    const auto COMBO_WINDOW = 500ms;

    auto clear_buffer = [&]() {
        display_buf[0] = ' ';
        display_buf[1] = ' ';
        display_buf[2] = ' ';
        display_buf[3] = ' ';
        pos = 0;
    };

    auto refresh_lcd = [&]() {
        lcd.clear();
        lcd.Home();
        lcd.printf("%s", display_buf);
    };

    auto show_message = [&](const char *msg, std::chrono::milliseconds t = 1000ms) {
        lcd.clear();
        lcd.Home();
        lcd.printf("%s", msg);
        ThisThread::sleep_for(t);
    };

    auto password_correct = [&]() -> bool {
        if (pos != 4) {
            return false;
        }
        return (display_buf[0] == password[0] &&
                display_buf[1] == password[1] &&
                display_buf[2] == password[2] &&
                display_buf[3] == password[3]);
    };

    auto enter_password_mode = [&]() {
        password_mode = true;
        clear_buffer();
        show_message("PASS", 1000ms);
        lcd.clear();
        lcd.Home();
        lcd.printf("    ");
        printf("Please enter password\n");
    };

    auto do_confirm = [&]() {
        if (!password_mode) {
            printf("Not in password mode\n");
            return;
        }

        if (password_correct()) {
            printf("Password correct\n");
            show_message("DONE", 1000ms);
        } else {
            printf("Password wrong\n");
            show_message("ERR ", 1000ms);
        }

        clear_buffer();
        password_mode = false;
        lcd.clear();
        lcd.Home();
        lcd.printf("    ");
    };

    auto do_clear = [&]() {
        clear_buffer();
        refresh_lcd();
        printf("Clear\n");
    };

    lcd.clear();
    lcd.Home();
    lcd.printf("    ");

    printf("Password system started\n");

    while (true) {
        // If * or # was pressed earlier but no combination was completed
        // within the time window, treat it as a single key action
        if (pending_special != NO_KEY &&
            (Kernel::Clock::now() - pending_time >= COMBO_WINDOW)) {

            if (pending_special == '*') {
                do_confirm();
            } else if (pending_special == '#') {
                if (password_mode) {
                    do_clear();
                } else {
                    printf("Clear ignored (not in password mode)\n");
                }
            }

            pending_special = NO_KEY;
        }

        key = keypad.ReadKey();

        if (key != NO_KEY) {

            // If a normal digit arrives while a special key is still pending,
            // process the pending special key first as a single-key action
            if (pending_special != NO_KEY && key != '*' && key != '#') {
                if (pending_special == '*') {
                    do_confirm();
                } else if (pending_special == '#') {
                    if (password_mode) {
                        do_clear();
                    }
                }
                pending_special = NO_KEY;
            }

            // Handle * and # combination logic
            if (key == '*' || key == '#') {
                if (pending_special != NO_KEY &&
                    key != pending_special &&
                    (Kernel::Clock::now() - pending_time <= COMBO_WINDOW)) {

                    pending_special = NO_KEY;
                    // Valid key combination detected: enter password mode
                    // Prevent duplicate entry
                    if(!password_mode){
                        enter_password_mode();
                    }
                } else {
                    // Store the special key temporarily and wait for a possible combination
                    pending_special = key;
                    pending_time = Kernel::Clock::now();
                }
            }

            // Handle normal digit input
            else if (key >= '0' && key <= '9') {
                if (!password_mode) {
                    printf("Ignored key %c (not in password mode)\n", key);
                } else {
                    // If already full, clear the previous 4 digits
                    // and start again with the new digit
                    if (pos >= 4) {
                        clear_buffer();
                    }

                    display_buf[pos] = key;
                    pos++;

                    refresh_lcd();
                    printf("Key pressed: %c\n", key);
                }
            }
        }

        ThisThread::sleep_for(20ms); 
    }
}
