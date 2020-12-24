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
    uint32 seqNumber;
    packet >> seqNumber;

    if(expectedSeqNum != seqNumber)
        return false;

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
    packet << ClientMessage::ConfirmPackets;
    packet << pendingAckNum.size();
    for (std::list<uint32>::iterator iter = pendingAckNum.begin(); iter != pendingAckNum.end(); ++iter)
        packet << *iter;
}

void DeliveryManager::processAckdSequenceNumbers(const InputMemoryStream& packet)
{
   //per cada seq. number trobes el paquet i crides la seva funcio de succes o failure.
    //elimines la delivery
    uint32 actualSeqNum = nextSeqNum;
    uint32 size;
    packet >> size; 

    while (size != 0)
    {
        uint32 seqNum;
        packet >> seqNum;
        for (std::list<Delivery*>::iterator iter = pendingDeliv.begin(); iter != pendingDeliv.end(); ++iter)
        {
            if (seqNum == (*iter)->sequenceNumber)
            {
                (*iter)->delegate->onDeliverySuccess(this);
                pendingDeliv.erase(iter);
                break;
            }
        }
        --size;
    }

    for (std::list<Delivery*>::iterator iter = pendingDeliv.begin(); iter != pendingDeliv.end(); )
    {
        if (actualSeqNum < (*iter)->sequenceNumber)
            break;

        (*iter)->delegate->onDeliveryFailure(this);
        iter = pendingDeliv.erase(iter);
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
            (*iter)->delegate->onDeliveryFailure(this);
            iter = pendingDeliv.erase(iter);
        }
        else
            ++iter;
    }
}

void DeliveryManager::Clear()
{
}

void DeliveryClient::onDeliverySuccess(DeliveryManager* deliveryManager)
{
}

void DeliveryClient::onDeliveryFailure(DeliveryManager* deliveryManager)
{
}

void DeliveryServer::onDeliverySuccess(DeliveryManager* deliveryManager)
{
}

void DeliveryServer::onDeliveryFailure(DeliveryManager* deliveryManager)
{
}
