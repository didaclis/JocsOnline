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

	void Clear();

private:

	uint32 nextSeqNum = 0;
	std::list<Delivery*> pendingDeliv;

	uint32 expectedSeqNum = 1;
	std::list<uint32> pendingAckNum;

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
	DeliveryDelegate* delegate = nullptr;
};

class DeliveryClient : public DeliveryDelegate
{
public:
	void onDeliverySuccess(DeliveryManager* deliveryManager);
	void onDeliveryFailure(DeliveryManager* deliveryManager);
};

class DeliveryServer : public DeliveryDelegate
{
public:
	void onDeliverySuccess(DeliveryManager* deliveryManager);
	void onDeliveryFailure(DeliveryManager* deliveryManager);
};
