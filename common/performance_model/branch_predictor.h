#ifndef BRANCH_PREDICTOR_H
#define BRANCH_PREDICTOR_H

#include <iostream>

#include "fixed_types.h"

class BranchPredictor
{
public:
   BranchPredictor();
   BranchPredictor(const String& name, core_id_t core_id);
   virtual ~BranchPredictor();

   virtual bool predict(bool indirect, IntPtr ip, IntPtr target) = 0;
   virtual void update(bool predicted, bool actual, bool indirect, IntPtr ip, IntPtr target) = 0;

   static UInt64 getMispredictPenalty();
   static BranchPredictor* create(core_id_t core_id);

   [[nodiscard]] UInt64 getNumCorrectPredictions() const { return m_correct_predictions; }
   [[nodiscard]] UInt64 getNumIncorrectPredictions() const { return m_incorrect_predictions; }

   void resetCounters();

protected:
   void updateCounters(bool predicted, bool actual);

private:
   UInt64 m_correct_predictions;
   UInt64 m_incorrect_predictions;

   static UInt64 m_mispredict_penalty;
};

#endif
