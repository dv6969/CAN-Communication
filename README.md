# CAN-Communication
**Reception Flow->**

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

Output of the above code:


![Image_Alt](https://github.com/dv6969/CAN-Communication/blob/main/Output_Receive_CAN.png?raw=true)

For More information about the CAN Protocol use the link->https://www.csselectronics.com/pages/can-bus-simple-intro-tutorial





**CAN Transmission flow->**


![Image_Alt](https://github.com/dv6969/CAN-Communication/blob/main/Transmission_workflow.png?raw=true)


🚗📡 AUTOSAR CAN Communication: Transmission Flow Explained with a Practical Example 🔌⚙️
Just like reception, in AUTOSAR the application never transmits CAN data directly.
All communication is routed through RTE and BSW to keep the system safe, portable, and scalable.

🔹 High-Level Transmission Flow
 →Application SWC
 
 → RTE
 
 → AUTOSAR COM
 
 → PDU Router (PduR)
 
 → CAN Interface (CanIf)
 
 → CAN Driver (MCAL)
 
 → CAN Bus


🧠 Example Used

Signal: DoorCmd

Meaning: Lock / Unlock command

Application value:

•	0 → Unlock

•	1 → Lock


1️⃣ Application Layer – Writing the Signal 🚘

The application produces a logical value and writes it via RTE:

uint8 DoorCmd = 1;  // Lock command

Rte_Write_PpDoorCmd_Value(DoorCmd);

✔ Application does not know:

•	CAN ID

•	Bit position

•	PDU layout

That complexity is hidden by AUTOSAR.


2️⃣ RTE – Bridging Application and BSW 🔌
The RTE:

•	Copies data into an internal RTE buffer

•	Calls the COM layer API

Internally, this results in:

Com_SendSignal(ComConf_ComSignal_DoorCmd, &DoorCmd);

✔ RTE ensures:

•	Application ↔ BSW isolation

•	Safe data transfer

•	Compile-time checked interfaces



3️⃣ AUTOSAR COM – Signal Handling & Packing 📦
What COM does:

•	Stores the signal in a COM signal buffer

•	Packs the signal bits into the configured IPDU

•	Decides when the PDU should be transmitted
📌 Example:

DoorCmd → Bit 0 of Byte 0 in DoorControl_IPDU




4️⃣ Transmission Triggering Methods (Very Important ⚡)

✅ Method 1: Immediate Transmission
(COM sends PDU instantly)
Triggered when:

•	Signal is configured as Triggered

•	Signal has Triggered on Change

Flow:
Rte_Write → Com_SendSignal → PduR_ComTransmit()

✔ Fast response

❌ Higher CPU usage



✅ Method 2: Deferred / Cyclic Transmission (Most Common)

•	Signal is stored in COM buffer

•	Actual transmission happens later in:

Com_MainFunctionTx();

Called by:

•	OS Task

•	Cyclic scheduler

✔ Deterministic

✔ CPU efficient

✔ Production-preferred method


5️⃣ PDU Router – Routing the Message 🔄

COM calls:

PduR_ComTransmit(PduId, &PduInfo);

✔ PduR routes the PDU to the correct bus:

•	CAN

•	LIN

•	FlexRay

•	Ethernet


6️⃣ CAN Interface & Driver – Sending on the Bus 🛠️
CanIf_Transmit();

Can_Write();

•	Hardware abstraction

•	CAN controller access

•	Final CAN frame is transmitted 🚗📡

🔗 Ways to Connect Application to BSW via RTE

🟢 1️⃣ Signal-Based Communication (Most Used)

Rte_Write_<Port>_<Signal>();

Rte_Read_<Port>_<Signal>();

✔ Simple

✔ Safe

✔ Ideal for most signals

________________________________________
🟢 2️⃣ Signal Group Communication

Used when multiple signals must be updated atomically:

Rte_Write_<Port>_<SignalGroup>();

✔ Consistency guaranteed

________________________________________
🟢 3️⃣ Runnable-Triggered Communication

•	Transmission occurs when a runnable executes

•	Often used in state machines

✔ Event-driven

________________________________________
🟢 4️⃣ Mode-Based Communication

•	Signals sent when ECU or SWC mode changes

•	Common in power and network management

________________________________________
✅ Key Takeaway 🚀

In AUTOSAR CAN transmission, the application only writes logical values via RTE.

COM handles signal packing and timing, while lower BSW layers take care of routing and hardware access.

This architecture ensures:

✔ Reusability

✔ Scalability

✔ Safety

✔ Network independence

**Transmission Output:**


![Image_Alt](https://github.com/dv6969/CAN-Communication/blob/main/output_transmission.png?raw=true)




