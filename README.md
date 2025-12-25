# CAN-Communication
Reception Flow->

![Image Alt](https://github.com/dv6969/CAN-Communication/blob/main/Autosar_can_receptation.png?raw=true)

🚗 AUTOSAR CAN Reception Flow (COM → RTE → Application)

When a CAN message is received on an AUTOSAR ECU, the application never talks directly to the CAN hardware or BSW.
Instead, the data flows through a well-defined stack that guarantees portability, safety, and scalability.

The uploaded diagram shows the reception path, from CAN Bus → Application SWC.


1️⃣ CAN Bus → CAN Driver (MCAL)

What happens

A CAN frame arrives on the bus, CAN hardware triggers an RX interrupt, CAN Driver copies the frame from hardware to software
CAN Bus → CAN HW/µC → CAN Driver (MCAL)
At this level:
Data is still a raw CAN frame, No AUTOSAR signals yet


2️⃣ CAN Driver → CAN Interface (CanIf)

The CAN Driver informs CanIf that a frame has been received:
CanIf_RxIndication(Hrh, &PduInfo);

What CanIf does

Checks received CAN ID

Maps CAN ID → Rx PDU ID

Forwards the PDU upward


3️⃣ CanIf → PDU Router (PduR)
PduR_CanIfRxIndication(RxPduId, &PduInfo);
What PduR does:

Routes the received PDU to the correct upper module

In this case → AUTOSAR COM

Important

PduR does no signal processing

It only routes PDUs


4️⃣ PduR → AUTOSAR COM (Com_RxIndication)
Com_RxIndication(RxPduId, &PduInfo);


This is where signal-level processing starts.


5️⃣ Inside COM – Unpacking the PDU

COM knows (from Com_Cfg.h):

Which signals belong to this PDU

Bit position, length, endianess

Example

Assume a CAN message contains an 8-bit signal DoorStatus at byte 0.
uint8 doorStatus = PduInfo->SduDataPtr[0];


6️⃣ COM Stores the Signal Internally

COM stores the value in its internal signal buffer:

Com_SignalBuffer[COM_SIG_DOOR_STATUS] = doorStatus;
Com_SignalUpdated[COM_SIG_DOOR_STATUS] = TRUE;


✔ Application still cannot access the signal
✔ Data is safe and consistent inside COM


7️⃣ Two Ways COM Delivers Data to RTE

The diagram shows two alternatives:

✅ A) Deferred Processing (Most Common)

Used in most production ECUs.

OS Task calls Com_ReceiveSignal()

uint8 doorStatus;
Com_ReceiveSignal(COM_SIG_DOOR_STATUS, &doorStatus);


What Com_ReceiveSignal() does

Copies data from COM buffer to caller buffer

Clears update flag (optional)


Std_ReturnType Com_ReceiveSignal(
    Com_SignalIdType SignalId,
    void* SignalDataPtr)
{
    *(uint8*)SignalDataPtr = Com_SignalBuffer[SignalId];
    return E_OK;
}


✅ B) Immediate Notification (Less Common)

Used for very fast reaction signals.

Inside Com_RxIndication():

Rte_Write_PpDoorStatus_DoorStatus(doorStatus);


COM directly notifies RTE without waiting for an OS task.


8️⃣ RTE – Writing into RTE Buffer

When RTE receives data:

Rte_Write_PpDoorStatus_DoorStatus(doorStatus);


Internally:

Rte_Buffer_DoorStatus = doorStatus;


✔ RTE owns the application-visible buffer
✔ Thread-safe, SWC-safe


9️⃣ Application SWC Reads the Signal

void DoorCtl_Runnable(void)
{
    uint8 status;
    Rte_Read_PpDoorStatus_DoorStatus(&status);

    if (status == 1)
    {
        // Door open logic
    }
}


Key point

Application never calls COM if the code is Autodar compilent 

Application never touches BSW if the code is Autodar compilent

Only Rte_Read() is used
