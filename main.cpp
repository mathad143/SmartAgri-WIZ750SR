/* SMART AGRICULTURE PROJECT Applicatios
# 3 phase detection
# soil moisture detection
# rfid access to enter the field (gate open and close)
# fensing security for entire field 
# security alarm for unauthorised access (for RFID and Fensing)
# Motor for water supply
*/
#include "mbed.h"
#include "MQTTEthernet.h"
#include "MQTTClient.h"
#define ECHO_SERVER_PORT   7
Serial uart(D1,D0);
// wiznet wiz750SR pin configuration
AnalogIn soilSensor(PC_8);
DigitalIn p1(PC_13);
DigitalIn p2(PC_12);
DigitalIn p3(PC_9);
float soil;
char *msg;
char uart_msg;
char buffer[15];
char *rf_on = "on";
char *rf_off = "off";
char *mot_on = "onmotor";
char *mot_off = "offmotor";

// call back function 
void messageArrived(MQTT::MessageData& md)
{
    strcpy(buffer,"");
    MQTT::Message &message = md.message;
    printf("Message arrived: qos %d, retained %d, dup %d, packetid %d\n", message.qos, message.retained, message.dup, message.id);
    printf("Payload %.*s\n", message.payloadlen, (char*)message.payload);
    sprintf(buffer,"%.*s",message.payloadlen,(char*)message.payload);
    printf("buffer value is %s\n",buffer);

}


void baud(int baudrate)
{
    Serial s(USBTX, USBRX);
    s.baud(baudrate);
}

int main (void)
{

    baud(9600);
    char *topic1 = "rfiddata";
    char *topic2 = "threephasebutton";
    printf("Wait a second...\r\n");

    MQTTEthernet ipstack = MQTTEthernet();

    MQTT::Client<MQTTEthernet, Countdown> client = MQTT::Client<MQTTEthernet, Countdown>(ipstack);
    
    // MQTT broker address and port no.
    char* hostname = "iot.eclipse.org";
    int port = 1883;

    int rc = ipstack.connect(hostname, port);
    if (rc == 0)
        printf("rc from TCP connect is %d\n", rc);



    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = "parents";

    if ((rc = client.connect(data)) == 0)
        printf("rc from MQTT connect is %d\n", rc);

    if ((rc = client.subscribe(topic1, MQTT::QOS1, messageArrived)) == 0)
        printf("rc from MQTT subscribe toppic is rfiddata %d\n", rc);


    if ((rc = client.subscribe(topic2, MQTT::QOS1, messageArrived)) == 0)
        printf("rc from MQTT subscribe toppic is threephasebutton %d\n", rc);

    MQTT::Message message;

    while (true) {

        // reading the soil moisture content from soil moisture sensor
        soil = soilSensor.read_u16();
        printf(" soil val is : %f\n",soil);

        // Data reading from arduino via UART
        if (uart.readable()) {
            printf("uart readable\n");
            uart_msg = uart.getc();

            if(uart_msg == 's') {               // Authorised access data of RFID
                msg = "Authorized Access";
                printf(" msg : %s\n", msg);
            }

            if(uart_msg == 'n' || uart_msg =='f') {        // unauthorised access data of RFID or Fensing
                msg = "Unauthorized Access";
                printf(" msg : %s\n", msg);

            }
            message.qos = MQTT::QOS0;
            message.retained = false;
            message.dup = false;
            message.payload = (void*)msg;
            message.payloadlen = strlen(msg);
            rc = client.publish("rfid", message);
            strcpy(msg,"");
            

        }
        // rfid command from mqtt "on"
        if(strcmp(buffer, rf_on)==0 ) {
            uart.putc('a');                 //sending the data to arduino via UART
            printf("rfid on data\n");
            strcpy(buffer,"");

        }
        // rfid command from mqtt "off"
        if(strcmp(buffer,rf_off)==0) {
            uart.putc('b');                 //sending the data to arduino via UART
            printf("rfid off data\n");
            strcpy(buffer,"");

        }
        if(strcmp(buffer,mot_on)==0) {
            uart.putc('c');                 //sending the data to arduino via UART
            printf("motor on data\n");
            strcpy(buffer,"");

        }

        if(strcmp(buffer,mot_off)==0) {
            uart.putc('d');                 //sending the data to arduino via UART
            printf("motor off data\n");
            strcpy(buffer,"");
        }

        // soil moisture and 3 phase current checking
        if(soil<250) {
            if((p1==1)&&(p2==1)&&(p3==1)) {     //three phase checking
                printf("3 phase detected\n");
                msg = "Low Moisture and 3 phase detected";
            } else {
                // printf("3 phase not detected\n");
                msg = "Low Moisture but 3 phase not detected";
            }


            message.qos = MQTT::QOS0;
            message.retained = false;
            message.dup = false;
            message.payload = (void*)msg;
            message.payloadlen = strlen(msg);
            rc = client.publish("threephase", message);
            strcpy(msg,"");
        }
        client.yield(500);
    }
}

