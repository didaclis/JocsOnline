#include "Networks.h"
#include "DeliveryManager.h"

//TODO(you): Reliability on top of UDP lab session

Delivery* DeliveryManager::writeSequenceNumber(OutputMemoryStream& packet)
{
    Delivery* newDelivery = new Delivery(); 
   newDelivery->sequenceNumber = nextSeqNum++;
    newDelivery->dispatchTime = Time.time;

    packet << newDelivery->sequenceNumber;

    pendingDeliv.push_back(newDelivery);

    return newDelivery;
}

bool DeliveryManager::processSequenceNumber(const InputMemoryStream& packet)
{
    //mirar que el numero de sequencia sigui el correcte i si no a pendre per el cul el packet
   //afegeixes el packet a la llista pendingAck
    for (std::list<int>::iterator iter = pendingAckNum.begin(); iter != pendingAckNum.end();)
    {
        if (expectedSeqNum > *iter)
            iter = pendingAckNum.erase(iter);
        else
            ++iter;
    }

    int seqNumber;
    packet >> seqNumber;

    if (expectedSeqNum != seqNumber)
    {
        return false;
    }

    expectedSeqNum = seqNumber + 1;
    pendingAckNum.push_back(seqNumber);

    return true;
    
}

bool DeliveryManager::hasSequenceNumbersPendingAck() const
{
    //mirar si tens pending ack
    return !pendingAckNum.empty();
}

void DeliveryManager::writeSequenceNumbersPendingAck(OutputMemoryStream& packet)
{
    //enviar packet amb tots els sequence numbers dels packets rebuts.
    packet << PROTOCOL_ID;
    packet << ClientMessage::ConfirmPackets;
    packet << expectedSeqNum - 1;
}

void DeliveryManager::processAckdSequenceNumbers(const InputMemoryStream& packet)
{
   //per cada seq. number trobes el paquet i crides la seva funcio de succes o failure.
    //elimines la delivery
    int lastPacketID;
    packet >> lastPacketID;
    
    for (std::list<Delivery*>::iterator iter = pendingDeliv.begin(); iter != pendingDeliv.end();)
    {
        if (lastPacketID >= (*iter)->sequenceNumber)
        {
            delete *iter;
            iter = pendingDeliv.erase(iter);
        }
        else
            break;
    }

    for (std::list<int>::iterator iter = pendingAckNum.begin(); iter != pendingAckNum.end();)
    {
        if (lastPacketID >= *iter)
        {
            iter = pendingAckNum.erase(iter);
        }
        else
            break;
    }
}


void DeliveryManager::processTimedOutPackets()
{
    uint32 actualSeqNum = nextSeqNum;

    //crides a la failure
    for (std::list<Delivery*>::iterator iter = pendingDeliv.begin(); iter != pendingDeliv.end();)
    {
        if (actualSeqNum < (*iter)->sequenceNumber)
            break;

        if (Time.time > PACKET_DELIVERY_TIMEOUT_SECONDS + (*iter)->dispatchTime)
        {
            delete* iter;
            iter = pendingDeliv.erase(iter);
        }
        else
            ++iter;
    }
}

void DeliveryManager::getAllPackets(std::list<OutputMemoryStream>& packets)
{
    for (std::list<Delivery*>::iterator iter = pendingDeliv.begin(); iter != pendingDeliv.end(); ++iter)
    {
        packets.push_back((*iter)->savedPacket);
    }
}

void DeliveryManager::Clear()
{
    for (std::list<Delivery*>::iterator iter = pendingDeliv.begin(); iter != pendingDeliv.end();)
    {
        delete[] * iter;
        *iter == nullptr;
    }
    pendingDeliv.clear();
    pendingAckNum.clear();
}

