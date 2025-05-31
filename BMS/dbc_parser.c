//made by chatGPT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/select.h>

#define MAX_MESSAGES 100
#define MAX_SIGNALS_PER_MESSAGE 50
#define LINE_BUFFER_SIZE 512

// Data structure for a Signal
typedef struct {
    char name[64];      // Signal name
    int start_bit;      // Start bit index
    int length;         // Length in bits
    int byte_order;     // Byte order (assume 1: little-endian, 0: big-endian)
    char sign;          // '+' for unsigned, '-' for signed
    float factor;       // Scaling factor
    float offset;       // Offset value
    float min;          // Minimum physical value
    float max;          // Maximum physical value
    char unit[32];      // Unit string
    char receiver[64];  // Receiver node name
    double latest_value; // Latest parsed physical value
} Signal;

// Data structure for a Message
typedef struct {
    int id;                  // Message ID (can identifier)
    char name[64];           // Message name
    int dlc;                 // Data Length Code
    char transmitter[64];    // Transmitter node
    char last_hex[17];       // Last received CAN frame in hex string
    Signal signals[MAX_SIGNALS_PER_MESSAGE]; // Array of signals
    int num_signals;         // Number of signals
} Message;

Message messages[MAX_MESSAGES];
int num_messages = 0;

// dbc 파일을 파싱해서 메시지와 시그널 정보를 저장하는 함수
void load_dbc(const char* filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open dbc file");
        exit(1);
    }
    char line[LINE_BUFFER_SIZE];
    Message *current_msg = NULL;
    while (fgets(line, LINE_BUFFER_SIZE, fp)) {
        if (line[0] == '\n' || line[0] == '\r')
            continue;
        // 메시지 정의 처리 (BO_)
        if (strncmp(line, "BO_", 3) == 0) {
            Message msg;
            memset(&msg, 0, sizeof(msg));
            int id, dlc;
            char name[64], transmitter[64];
            // 포맷: BO_ <id> <message_name>: <dlc> <transmitter>
            if (sscanf(line, "BO_ %d %[^:]: %d %s", &id, name, &dlc, transmitter) == 4) {
                msg.id = id;
                strncpy(msg.name, name, sizeof(msg.name) - 1);
                msg.dlc = dlc;
                strncpy(msg.transmitter, transmitter, sizeof(msg.transmitter) - 1);
                msg.num_signals = 0;
                messages[num_messages++] = msg;
                current_msg = &messages[num_messages - 1];
            }
        }
        // 시그널 정의 처리 (SG_) 및 현재 메시지 내 등록
        else if (strncmp(line, " SG_", 4) == 0 && current_msg != NULL) {
            Signal sig;
            memset(&sig, 0, sizeof(sig));
            sig.latest_value = 0.0; // initialize latest value
            char *ptr = line + 4;
            char signal_name[64];
            if (sscanf(ptr, " %s", signal_name) < 1)
                continue;
            strncpy(sig.name, signal_name, sizeof(sig.name) - 1);
            // Parse the part after the ':' character
            char *colon = strstr(ptr, ":");
            if (!colon)
                continue;
            colon++; // start after ':'
            
            /*
               Expected format:
               <start_bit>|<length>@<byte_order><sign> (<factor>,<offset>) [<min>|<max>] "<unit>" <receiver>
               Example: 0|8@1+ (1,0) [0|100] "%" BMS
               For signals with an empty unit (like MinTempID and MaxTempID), the sscanf may fail to match
               the unit string. We thus try an alternative scan pattern that explicitly matches "" for empty unit.
            */
            int start_bit, length, byte_order;
            char sign_char;
            float factor, offset, min, max;
            char unit[32];
            char receiver[64];
            
            int scanned = sscanf(colon, " %d|%d@%d%c (%f,%f) [%f|%f] \"%31[^\"]\" %s",
                                &start_bit, &length, &byte_order, &sign_char,
                                &factor, &offset, &min, &max, unit, receiver);
            if (scanned == 10) {
                // Successfully parsed with non-empty unit
            } else {
                // Try alternative pattern for empty unit: explicitly match "" and then receiver
                scanned = sscanf(colon, " %d|%d@%d%c (%f,%f) [%f|%f] \"\" %s",
                                &start_bit, &length, &byte_order, &sign_char,
                                &factor, &offset, &min, &max, receiver);
                if (scanned == 9) {
                    unit[0] = '\0';
                } else {
                    continue;
                }
            }
            
            sig.start_bit = start_bit;
            sig.length = length;
            sig.byte_order = byte_order;
            sig.sign = sign_char;
            sig.factor = factor;
            sig.offset = offset;
            sig.min = min;
            sig.max = max;
            strncpy(sig.unit, unit, sizeof(sig.unit) - 1);
            strncpy(sig.receiver, receiver, sizeof(sig.receiver) - 1);
            current_msg->signals[current_msg->num_signals++] = sig;
        }
    }
    fclose(fp);
}

// CAN 프레임 데이터(8바이트)에서 시그널 값을 추출하는 함수 (little-endian 방식으로 처리)
double extract_signal(uint8_t data[8], Signal *sig) {
    uint64_t raw = 0;
    // 8바이트를 little-endian 정수형으로 결합
    for (int i = 0; i < 8; i++) {
        raw |= ((uint64_t) data[i]) << (8 * i);
    }
    uint64_t mask = ((uint64_t)1 << sig->length) - 1;
    uint64_t extracted = (raw >> sig->start_bit) & mask;
    int64_t value = extracted;
    if (sig->sign == '-') {
        // 부호 확장 (signed value)
        int64_t sign_bit = 1LL << (sig->length - 1);
        if (extracted & sign_bit) {
            value = extracted - (1LL << sig->length);
        }
    }
    double physical = value * sig->factor + sig->offset;
    return physical;
}

// dbc 메시지 정의를 반환하는 함수
Message* find_message(int id) {
    for (int i = 0; i < num_messages; i++) {
        if (messages[i].id == id) {
            return &messages[i];
        }
    }
    return NULL;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s <dbc_file> <can_interface>\n", argv[0]);
        return 1;
    }
    const char* dbc_file = argv[1];
    const char* can_interface = argv[2];

    // Load DBC file
    load_dbc(dbc_file);
    printf("Loaded %d messages from dbc file: %s\n", num_messages, dbc_file);

    // Open SocketCAN interface
    int s;
    if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
        perror("Error while opening socket");
        return 1;
    }
    struct ifreq ifr;
    strncpy(ifr.ifr_name, can_interface, IFNAMSIZ - 1);
    if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) {
        perror("Error getting interface index");
        return 1;
    }
    struct sockaddr_can addr;
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Error in socket bind");
        return 1;
    }
    printf("Listening on CAN interface: %s ...\n", can_interface);

    struct can_frame frame;
    fd_set readfds;
    struct timeval timeout;

    // Main loop: update display continuously
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(s, &readfds);
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000; // 100ms timeout

        int ret = select(s + 1, &readfds, NULL, NULL, &timeout);
        if (ret > 0 && FD_ISSET(s, &readfds)) {
            int nbytes = read(s, &frame, sizeof(struct can_frame));
            if (nbytes == sizeof(struct can_frame)) {
                Message *msg = find_message(frame.can_id);
                if (msg) {
                    sprintf(msg->last_hex, "%02X%02X%02X%02X%02X%02X%02X%02X", frame.data[0], frame.data[1], frame.data[2], frame.data[3], frame.data[4], frame.data[5], frame.data[6], frame.data[7]);
                    for (int j = 0; j < msg->num_signals; j++) {
                        Signal *sig = &msg->signals[j];
                        sig->latest_value = extract_signal(frame.data, sig);
                    }
                }
            }
        }
        // Clear screen and update display
        printf("\033[H\033[J");
        printf("=========== CAN Data Display ===========\n");
        for (int i = 0; i < num_messages; i++) {
            Message *msg = &messages[i];
            printf("Message ID: %d | Name: %s | DLC: %d | Transmitter: %s\n", 
                msg->id, msg->name, msg->dlc, msg->transmitter);
            printf("  Received Hex: %s\n", msg->last_hex);
            for (int j = 0; j < msg->num_signals; j++) {
                Signal *sig = &msg->signals[j];
                printf("  Signal: %s = %.2f %s\n", sig->name, sig->latest_value, sig->unit);
            }
            printf("---------------------------------------\n");
        }
        fflush(stdout);
    }
    close(s);
    return 0;
}