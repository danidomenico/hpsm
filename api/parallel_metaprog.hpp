#pragma once

#include <type_traits>

namespace metaprog {

//Enable_if
template<bool B, typename T = void>
using Enable_if = typename std::enable_if<B,T>::type;

//Conditional
template<bool B, typename T, typename F>
using Conditional = typename std::conditional<B,T,F>::type;

//Select
class Nil {};

template<int I, typename T1 =Nil, typename T2 =Nil, typename T3 =Nil>
struct select_own;

template<int I, typename T1 =Nil, typename T2 =Nil, typename T3 =Nil>
using Select = typename select_own<I,T1,T2,T3>::type;

// Specializations for 0-3:
template<typename T1, typename T2, typename T3>
struct select_own<0,T1,T2,T3> { using type = T1; }; // specialize for N==0

template<typename T1, typename T2, typename T3>
struct select_own<1,T1,T2,T3> { using type = T2; }; // specialize for N==1

template<typename T1, typename T2, typename T3>
struct select_own<2,T1,T2,T3> { using type = T3; }; // specialize for N==2

} //namespace metaprog