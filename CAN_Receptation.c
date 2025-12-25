#include <stdio.h>
#include <stdint.h>

//Each signal in com has seperate id here we assume that the doorstatus id is 0 in real autosar the id is generated with the hep of arxml
#define COM_SIG_DOOR_STATUS  0

uint8_t Com_SignalBuffer[1];
uint8_t Com_SignalUpdated[1];

// Called when PDU is received 
void Com_RxIndication(uint8_t* pduData)
{
    // Unpack signal (DoorStatus at byte 0)
    uint8_t doorStatus = pduData[0];

    // Store signal internally
    Com_SignalBuffer[COM_SIG_DOOR_STATUS] = doorStatus;
    Com_SignalUpdated[COM_SIG_DOOR_STATUS] = 1;

    printf("[COM] PDU received, DoorStatus unpacked = %d\n", doorStatus);
}

// Application-safe API
int Com_ReceiveSignal(uint8_t signalId, uint8_t* value)
{
    if (Com_SignalUpdated[signalId])
    {
        *value = Com_SignalBuffer[signalId];
        Com_SignalUpdated[signalId] = 0;
        return 0; // E_OK
    }
    return 1; // E_NOT_OK 
}

//Rte Layer functions
uint8_t Rte_Buffer_DoorStatus;

void Rte_Write_DoorStatus(uint8_t value)
{
    Rte_Buffer_DoorStatus = value;
    printf("[RTE] DoorStatus written to RTE buffer = %d\n", value);
}

void Rte_Read_DoorStatus(uint8_t* value)
{
    *value = Rte_Buffer_DoorStatus;
}

//Appilcation layer Code 

void DoorControl_Runnable(void)
{
    uint8_t doorStatus;
    Rte_Read_DoorStatus(&doorStatus);

    if (doorStatus == 1)
        printf("[APP] Door is OPEN\n");
    else
        printf("[APP] Door is CLOSED\n");
}

//Code start from here Main funcation
int main(void)
{
    //Simulated received CAN PDU 
    uint8_t canPdu[8] = {1, 0, 0, 0, 0, 0, 0, 0};
    printf("CAN Reception Simulation:\n");
    
    // COM receives PDU
    Com_RxIndication(canPdu);
    
    //  OS task pulls signal usuall the com receive signal definition is present in OS.c files
    uint8_t doorStatus;
    if (Com_ReceiveSignal(COM_SIG_DOOR_STATUS, &doorStatus) == 0)
    {
       // RTE write 
        Rte_Write_DoorStatus(doorStatus);
    }
   // Application runnable 
    DoorControl_Runnable();
    return 0;
}

