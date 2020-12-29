#pragma once
#include<list>

struct Delivery;

class DeliveryManager
{
public:
	// For senders to write a new seq. numbers into a packet
	Delivery* writeSequenceNumber(OutputMemoryStream& packet);
	// For receivers to process the seq. number from an incoming packet
	bool processSequenceNumber(const InputMemoryStream& packet);
	// For receivers to write ack'ed seq. numbers into a packet	
	bool hasSequenceNumbersPendingAck() const;
	void writeSequenceNumbersPendingAck(OutputMemoryStream& packet);
	// For senders to process ack'ed seq.numbers from a packet
	void processAckdSequenceNumbers(const InputMemoryStream& packet);
	void processTimedOutPackets();

	void getAllPackets(std::list<OutputMemoryStream>& packets);

	void Clear();

private:

	int nextSeqNum = 0;
	std::list<Delivery*> pendingDeliv;

	int expectedSeqNum = 0;
	std::list<int> pendingAckNum;

};

// TODO(you): Reliability on top of UDP lab session
class DeliveryDelegate
{
public:
	virtual void onDeliverySuccess(DeliveryManager* deliveryManager) = 0;
	virtual void onDeliveryFailure(DeliveryManager* deliveryManager) = 0;
};

struct Delivery
{
	uint32 sequenceNumber = 0;
	double dispatchTime = 0.0;
	OutputMemoryStream savedPacket;
};

