#ifndef SATURATING_PREDICTOR_H
#define SATURATING_PREDICTOR_H

#include <cstdint>
#include "fixed_types.h"

// From http://www.josuttis.com/tmplbook/meta/pow3.hpp.html
template <int N> class Pow2
{
public:
   enum { pow = 2 * Pow2<N-1>::pow };
};

template <> class Pow2<0>
{
public:
   enum { pow = 1 };
};

#define SAT_PRED_DEBUG 0

template <unsigned n>
class SaturatingPredictor {

public:

   explicit SaturatingPredictor(UInt32 initial_value = 0)
   {
      m_counter = initial_value;
   }

   bool predict()
   {
      return (m_counter >= 0);
   }

   void reset(bool prediction = false)
   {
      if (prediction)
      {
         // Make this counter favor taken
         m_counter = Pow2<n-1>::pow - 1;
      }
      else
      {
         // Make this counter favor not-taken
         m_counter = -Pow2<n-1>::pow;
      }
   }

   // update
   // true - branch taken
   // false - branch not-taken
   void update(bool actual)
   {
      if (actual)
      {
         // Move towards taken
         ++(*this);
      }
      else
      {
         // Move towards not-taken
         --(*this);
      }
   }

   // ONLY works if 2-bit; This late, I can't be bothered to generalise this
   void resetMsb()
   {
       m_counter &= 0b01;
   }

   void resetLsb()
   {
       m_counter &= 0b10;
   }

   SaturatingPredictor& operator++()
   {
#if SAT_PRED_DEBUG
      cout << "operator++ called! Old val = " << (int) m_counter;
#endif
      // Maximum signed value for n bits is (2^(n-1)-1)
      if (m_counter != (Pow2<n-1>::pow - 1))
      {
         ++m_counter;
      }
#if SAT_PRED_DEBUG
      cout << " New val = " << (int) m_counter << endl;
#endif
      return *this;
   }

   SaturatingPredictor& operator--()
   {
#if SAT_PRED_DEBUG
      cout << "operator-- called! Old val = " << (int) m_counter;
#endif
      // Minimum signed value for n bits is -(2^(n-1))
      if (m_counter != (-Pow2<n-1>::pow))
      {
         --m_counter;
      }
#if SAT_PRED_DEBUG
      cout << " New val = " << (int) m_counter << endl;
#endif
      return *this;
   }

   bool operator==(SaturatingPredictor<n> const& other) const
   {
       return m_counter == other.m_counter;
   }

private:

   int8_t m_counter;

};

#endif /* SATURATING_PREDICTOR_H */
