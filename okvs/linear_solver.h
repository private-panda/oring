/*In previous version (https://github.com/asu-crypto/mPSI/blob/paxos/libPaXoS/ObliviousDictionary.h), the authors use a linear sover in linbox.
However, bugs show occasionally. Therefore, We write a linear solver by using Gauss elimination here.*/

#pragma once

#include <vector>
#include <NTL/GF2E.h>
#include <NTL/mat_GF2E.h>
#include <NTL/GF2XFactoring.h>

#include <NTL/ZZ.h>

//using Gauss solver
NTL::mat_GF2E solver(NTL::mat_GF2E &A, const NTL::vec_GF2E &b, NTL::vec_GF2E &x);

int check_encoding(NTL::mat_GF2E &A, const NTL::vec_GF2E &b, const NTL::vec_GF2E &x);
