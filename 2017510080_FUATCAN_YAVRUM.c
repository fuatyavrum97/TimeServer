#include <arpa/inet.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// Check for a char that is candidate to be a padding character
int checkForPadding(char _char) {
    // All valid padding characters
    char paddings[] = {'-', '_', '0', '+', '^', '#'};
    int sizeOfPaddings = strlen(paddings);
    for (int i = 0; i < sizeOfPaddings; i++) {
        if (_char == paddings[i]) return 1;
    }
    return 0;
}

// Check for format string part that is documented in the assignment documentation
char *checkForFormat(char *expression) {
    // All valid formats with different lengths
    char *formats[] = {"a", "A", "b", "B", "c", "C", "d", "D", "e", "F", "g", "G", "h", "H", "I", "j", "k", "l", "m", "M", "n", "N", "p", "P", "q", "r", "R", "s", "S", "t", "T", "u", "U", "V", "w", "W", "x", "X", "y", "Y", "z", "Z", ":z", "::z", ":::z", "%"};
    int sizeOfFormats = sizeof(formats) / sizeof(formats[0]);

    // Compare strings return a result
    for (int i = 0; i < sizeOfFormats; i++) {
        if (strncmp(expression, formats[i], strlen(formats[i])) == 0) {
            return formats[i];
        }
    }
    return "";
}

// Main function
int main(int argc, char *argv[]) {
    // Create socket
    int socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        puts("Could not create socket");
        return 1;
    }

    // Create internet socket address of server
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(8888);

    // Bind the local address to socket
    if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
        puts("Binding failed");
        return 1;
    }
    puts("Socket is binded");

    // Prepare to accept connections on socket
    listen(socket_desc, 3);

    puts("Waiting for incoming connections...");

    // Accept a new connection on socket
    struct sockaddr_in client;
    int c = sizeof(struct sockaddr_in);
    int new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t *)&c);
    if (new_socket == -1) {
        puts("Accept failed");
        return 1;
    }
    puts("Connection accepted");

    // Write message to socket
    char *message = "\n\nHello client, I received your connection.\n\n";
    write(new_socket, message, strlen(message));

    // Read message from socket
    char buffer[2000];
    while (1) {
        // Empty the buffer
        memset(buffer, '\0', sizeof(buffer));

        // Read message from socket
        if (recv(new_socket, buffer, sizeof(buffer), 0) == -1) {
            puts("Recv failed");
            break;
        }
        // Clean useless last characters ascii 13 and 10 CLR
        buffer[strlen(buffer) - 1] = '\0';

        // Validations
        // Length validation
        if (strlen(buffer) < 12) {
            write(new_socket, "INCORRECT REQUEST\n", 19);
            continue;
        }
        // GET_DATE validation
        if (strncmp(buffer, "GET_DATE ", 9) != 0) {
            write(new_socket, "INCORRECT REQUEST\n", 19);
            continue;
        }

        // Format validation
        // Validate
        /**
         * The algorith to validate the format string is shown below
         * 
         * TYPES:
         * PERCENT -> "%"
         * PADDING -> "-" "_" "0"
         * FORMAT -> "D" "::z" "%"
         * 
         * VALID EXPRESSION SHAPE:
         * PERCENT [PADDING IF EXISTS] FORMAT
         * 
         * ALGORITHM:
         * AlphaNumeric
         * "a" "b" "1" "2" -> invalid
         * Percent sign 
         * "%"
         *      Valid format
         *      "D" "::z" "%" -> valid
         *      Padding
         *      "_" "-" "0"
         *          Valid format
         *          "D" ":z" "%" -> valid
         *          Else
         *          -> invalid
         *      No More Character
         *      -> invalid
         * Else (NonAlphaNumericNonPercent)
         * "," "/" -> valid
        */

        int error = 0;
        int formatFound = 0;
        char *buffer_only_formatted = buffer + 9;
        for (int i = 0; i < strlen(buffer_only_formatted); i++) {
            if (isalnum(buffer_only_formatted[i])) {
                error = 1;
            } else if (buffer_only_formatted[i] == '%') {
                char *format = checkForFormat(buffer_only_formatted + i + 1);
                if (strlen(format) > 0) {
                    i += strlen(format);
                    formatFound = 1;
                } else if (checkForPadding(buffer_only_formatted[i + 1])) {
                    format = checkForFormat(buffer_only_formatted + i + 2);
                    if (strlen(format) > 0) {
                        i += 1 + strlen(format);
                        formatFound = 1;
                    } else {
                        error = 1;
                        i += 1;
                    }
                } else {
                    error = 1;
                }
            } else {
            }
        }

        // Check for validation of the formatted string before translate it
        if (formatFound == 0 || error == 1) {
            // Send error
            write(new_socket, "INCORRECT REQUEST\n", 19);
            continue;
        }

        // Create a file pointer to run the date command like in terminal
        FILE *pp;
        // Create the string that will be run to get the actual date
        char result[2048] = "date \"+";
        strcat(result, buffer_only_formatted);
        strcat(result, "\"");
        // Run the query
        pp = popen(result, "r");
        // Success
        if (pp != NULL) {
            // Clear buffer
            char *line;
            char buffer2[1000];
            memset(buffer2, '\0', sizeof(buffer2));

            // Get the line of the result
            line = fgets(buffer2, sizeof buffer2, pp);
            if (line == NULL) continue;

            // Send the result to the client
            write(new_socket, line, strlen(line));

            pclose(pp);
        } else {
            // Inform user
            write(new_socket, "INCORRECT REQUEST\n", 19);
            continue;
        }
    }

    // Close sockets
    close(socket_desc);
    close(new_socket);

    return 0;
}
