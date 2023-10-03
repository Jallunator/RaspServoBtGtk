#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <unistd.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <softPwm.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#define SERVO_PIN 12 //GPIO 12, Board 32, Wiringpi 26


int handleCommands(void);
int client_socket;
int server_socket;


/****************** JOYSTICK START ************/
#define SENSOR_1_INPUT 22
#define SENSOR_2_INPUT 23
#define SWITCH_INPUT   24
int joystickRunning = 1;
int currPwm = 0;
void *joystickThread(void *arg) {
    
    softPwmCreate(SERVO_PIN  , 0, 200);
    while (joystickRunning) {
        // Get signals from Arduino as digital input values.
        int X_VALUE = digitalRead(SENSOR_1_INPUT);
        int Y_VALUE = digitalRead(SENSOR_2_INPUT);
        int SWITCH_VALUE = digitalRead(SWITCH_INPUT);
        // Print values.
        printf("SENSOR_1_VALUE: %d\n", X_VALUE);
        printf("SENSOR_2_VALUE: %d\n", Y_VALUE);
        printf("SWITCH_VALUE: %d\n", SWITCH_VALUE);

        if (!SWITCH_VALUE)break;
        
        printf("curr %d\n", currPwm);

         if (!X_VALUE) {//LEFT
            currPwm -= 1;
            if (currPwm < 1) {
                currPwm = 1;
            }
            softPwmWrite(SERVO_PIN, currPwm);
        }

         if (!Y_VALUE) {//RIGHT
            currPwm += 1;
            if (currPwm > 23) {
                currPwm = 23;
            }
            softPwmWrite(SERVO_PIN, currPwm);
        }
        delay(1000);
    }
    softPwmStop(SERVO_PIN);

    return NULL;
}

int joyStick(void) {
    
    wiringPiSetupGpio();

    // SET UP PINS
    pinMode(SENSOR_1_INPUT, INPUT);
    pinMode(SENSOR_2_INPUT, INPUT);
    pinMode(SWITCH_INPUT, INPUT);

    // THREAD FOR JOYSTICK CONTROLS
    pthread_t joystickThreadHandle;
    pthread_create(&joystickThreadHandle, NULL, joystickThread, NULL);

    // WAIT JOYSTICK TO FINISH
    pthread_join(joystickThreadHandle, NULL);
	
    return 0;
}
/******************  JOYSTICK END ***********/


int servoMove(char *s){

    if (wiringPiSetupGpio() == -1) {
        fprintf(stderr, "Failed to initialize WiringPi\n");
        return 1;
    }

    // Initialize hardware PWM
    pinMode(SERVO_PIN, PWM_OUTPUT);
    pwmSetMode(PWM_MODE_MS);
    pwmSetClock(192);  // Set the clock divisor to achieve a base frequency of 50 Hz (20 ms period)
    pwmSetRange(2000);  // Set the range to achieve the desired pulse width range

    int min = 1;
    int max = 220;

    if (strcmp(s, "open") == 0){
        pwmWrite(SERVO_PIN, max);
        currPwm = max;
        delay(1000);
    }
    else if (strcmp(s, "close") == 0){
        pwmWrite(SERVO_PIN, min);
        currPwm = min;
        delay(1000);
    }
    else if (strcmp(s, "half") == 0){
        pwmWrite(SERVO_PIN, max / 2);
        currPwm = max / 2;
        delay(1000);
    }
    else if (strcmp(s, "left") == 0){
        if (currPwm > min){
            currPwm--;
            pwmWrite(SERVO_PIN,currPwm);
            delay(100);
        }
    }
    else if (strcmp(s, "right") == 0){
        if (currPwm > min){
            currPwm++;
            pwmWrite(SERVO_PIN, currPwm);
            delay(100);
        }
    }
    pwmWrite(SERVO_PIN, 0);

    return 0;
}

int handleCommands(void){
   
   //RESEIVE MSG
	char *successmsg = "Success !\n";
    char buffer[1024];
    int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytes_received <= 0) {
        return -1;
    }
    buffer[bytes_received] = '\0';

    //CHECK COMMAND
    if(strcmp(buffer, "open") == 0){
        if (servoMove("open")){send(client_socket, strerror(errno), strlen(strerror(errno)), 0); return -1;}  
		send(client_socket, successmsg, strlen(successmsg), 0);
    }
    else if (strcmp(buffer, "close") == 0){
        if (servoMove("close")){send(client_socket, strerror(errno), strlen(strerror(errno)), 0); return -1;}  
		send(client_socket, successmsg, strlen(successmsg), 0);
    }
	else if (strcmp(buffer, "half") == 0){
		if (servoMove("half")){send(client_socket, strerror(errno), strlen(strerror(errno)), 0); return -1;}    
		send(client_socket, successmsg, strlen(successmsg), 0);
	}
    else if (strcmp(buffer, "spinleft") == 0){    
        if (servoMove("left")){send(client_socket, strerror(errno), strlen(strerror(errno)), 0); return -1;}    
        send(client_socket, successmsg, strlen(successmsg), 0);
    }
    else if (strcmp(buffer, "spinright") == 0){    
        if (servoMove("right")){send(client_socket, strerror(errno), strlen(strerror(errno)), 0); return -1;}    
		printf("JEEEEEEEEEEEEEEE\n");
        send(client_socket, successmsg, strlen(successmsg), 0);
    }
    else if (strcmp(buffer, "joystick") == 0){
        if (joyStick()){send(client_socket, strerror(errno), strlen(strerror(errno)), 0); return -1;}    
        send(client_socket, successmsg, strlen(successmsg), 0);
    }
	else{
        send(client_socket, strerror(errno), strlen(strerror(errno)), 0); 
        return 1;
    }

    return 0;
}


int main(void) {

    // Create a Bluetooth RFCOMM socket.
    server_socket = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

    // Bind to "C8:7F:54:98:B6:09" rfcomm channel 4
    struct sockaddr_rc server_addr;
    server_addr.rc_family = AF_BLUETOOTH;
    str2ba("B8:27:EB:BB:5A:DA", &server_addr.rc_bdaddr);
    server_addr.rc_channel = 4;
    bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));

    //WAIT CONNECTION
    listen(server_socket, 1);

    // ACCEPT CONNECTION
    struct sockaddr_rc client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_socket != -1){printf("Connected !\n");}

    // COMMUNICATE
    while (1) {
        
		if(handleCommands() == -1)return 1;
 
    }

    // Close the sockets
    close(client_socket);
    close(server_socket);

    joystickRunning = 0;
    return 0;
}
