#include "Networks.h"
#include "DeliveryManager.h"

//TODO(you): Reliability on top of UDP lab session

Delivery* DeliveryManager::writeSequenceNumber(OutputMemoryStream& packet)
{
    nextSeqNum++;
    Delivery* newDelivery = new Delivery; 
    newDelivery->sequenceNumber = nextSeqNum;
    newDelivery->dispatchTime = Time.time;

    packet << newDelivery->sequenceNumber;

    pendingDeliv.emplace(newDelivery,packet);

    return newDelivery;
}

//client
bool DeliveryManager::processSequenceNumber(const InputMemoryStream& packet)
{
    //mirar que el numero de sequencia sigui el correcte i si no a pendre per el cul el packet
   //afegeixes el packet a la llista pendingAck
    uint32 seqNumber;
    packet >> seqNumber;

    if (expectedSeqNum != seqNumber)
    {
        return false;
    }

    ++expectedSeqNum;
    pendingAckNum.push_back(seqNumber);
    return true;
    
}

bool DeliveryManager::hasSequenceNumbersPendingAck() const
{
    //mirar si tens pending ack
    return !pendingAckNum.empty();
}

//client
void DeliveryManager::writeSequenceNumbersPendingAck(OutputMemoryStream& packet)
{
    //enviar packet amb tots els sequence numbers dels packets rebuts.
    packet << pendingAckNum.size();
    for (std::list<uint32>::iterator iter = pendingAckNum.begin(); iter != pendingAckNum.end(); ++iter)
        packet << *iter;
    pendingAckNum.clear();

}

//server
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
        for (std::map<Delivery*,OutputMemoryStream>::iterator iter = pendingDeliv.begin(); iter != pendingDeliv.end(); ++iter)
        {
            if (seqNum == (*iter).first->sequenceNumber)
            {
                (*iter).first->delegate->onDeliverySuccess(this);
                pendingDeliv.erase(iter);
                break;
            }
        }
    }

    for (std::map<Delivery*, OutputMemoryStream>::iterator iter = pendingDeliv.begin(); iter != pendingDeliv.end(); )
    {
        if (actualSeqNum < (*iter).first->sequenceNumber)
            break;

        (*iter).first->delegate->onDeliveryFailure(this);
        //iter = pendingDeliv.erase(iter);
        ++iter;
    }
}

//server
void DeliveryManager::processTimedOutPackets()
{
    uint32 actualSeqNum = nextSeqNum;

    //crides a la failure
    for (std::map<Delivery*, OutputMemoryStream>::iterator iter = pendingDeliv.begin(); iter != pendingDeliv.end();)
    {
        if (actualSeqNum < (*iter).first->sequenceNumber)
            break;

        if (Time.time > PACKET_DELIVERY_TIMEOUT_SECONDS + (*iter).first->dispatchTime)
        {
            (*iter).first->delegate->onDeliveryFailure(this);
            //iter = pendingDeliv.erase(iter);
        }
        //else
            ++iter;
    }
}

void DeliveryManager::GetPendingPackets(std::vector<OutputMemoryStream>& p)
{
    for (std::map<Delivery*, OutputMemoryStream>::iterator iter = pendingDeliv.begin(); iter != pendingDeliv.end();++iter)
    {
        p.push_back((*iter).second);
    }
}

void DeliveryManager::SavePacket(Delivery* deliv, const OutputMemoryStream& packet)
{
    pendingDeliv[deliv] = packet;
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
