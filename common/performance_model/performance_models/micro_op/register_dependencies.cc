#include "register_dependencies.h"
#include "dynamic_micro_op.h"

RegisterDependencies::RegisterDependencies()
{
   clear();
}

void RegisterDependencies::setDependencies(DynamicMicroOp& microOp, uint64_t lowestValidSequenceNumber)
{
   // Create the dependencies for the microOp
   for(uint32_t i = 0; i < microOp.getMicroOp()->getSourceRegistersLength(); i++)
   {
      uint32_t sourceRegister = microOp.getMicroOp()->getSourceRegister(i);
      uint64_t producerSequenceNumber;
      LOG_ASSERT_ERROR(sourceRegister < XED_REG_LAST, "Source register src[%u]=%u is invalid", i, sourceRegister);
      if ((producerSequenceNumber = producers[sourceRegister]) != INVALID_SEQNR)
      {
         if (producerSequenceNumber >= lowestValidSequenceNumber)
         {
            microOp.addDependency(producerSequenceNumber);
         }
         else
         {
            producers[sourceRegister] = INVALID_SEQNR;
         }
      }
   }

   // Update the producers
   for(uint32_t i = 0; i < microOp.getMicroOp()->getDestinationRegistersLength(); i++)
   {
      uint32_t destinationRegister = microOp.getMicroOp()->getDestinationRegister(i);
      LOG_ASSERT_ERROR(destinationRegister < XED_REG_LAST, "Destination register dst[%u] = %u is invalid", i, destinationRegister);
      producers[destinationRegister] = microOp.getSequenceNumber();
   }

}

void RegisterDependencies::clear()
{
   for(uint32_t i = 0; i < XED_REG_LAST; i++)
   {
      producers[i] = INVALID_SEQNR;
   }
}