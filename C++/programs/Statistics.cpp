////////////////////////////////////////////////////////////////////////////////
//! @file	: Statistics.cpp
//! @date   : Jul 2013
//!
//! @brief  : Statistical reductions on images
//! 
//! Copyright (C) 2013 - CRVI
//!
//! This file is part of OpenCLIPP.
//! 
//! OpenCLIPP is free software: you can redistribute it and/or modify
//! it under the terms of the GNU Lesser General Public License version 3
//! as published by the Free Software Foundation.
//! 
//! OpenCLIPP is distributed in the hope that it will be useful,
//! but WITHOUT ANY WARRANTY; without even the implied warranty of
//! MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//! GNU Lesser General Public License for more details.
//! 
//! You should have received a copy of the GNU Lesser General Public License
//! along with OpenCLIPP.  If not, see <http://www.gnu.org/licenses/>.
//! 
////////////////////////////////////////////////////////////////////////////////

#include "Programs/Statistics.h"


#define KERNEL_RANGE(src_img) GetRange(src_img), GetLocalRange()
#define SELECT_NAME(name, src_img) SelectName( #name , src_img)

#include "kernel_helpers.h"

#include "StatisticsHelpers.h"


using namespace std;

namespace OpenCLIPP
{


// Statistics
void Statistics::PrepareBuffer(const ImageBase& Image)
{
   size_t NbGroups = (size_t) GetNbGroups(Image);

   // We need space for 4 channels + another space for the number of pixels
   size_t BufferSize = NbGroups * (4 + 1);

   if (m_PartialResultBuffer != nullptr &&
      m_PartialResultBuffer->Size() == BufferSize * sizeof(float) &&
      m_PartialResult.size() == BufferSize)
   {
      return;
   }

   m_PartialResult.assign(BufferSize, 0);

   m_PartialResultBuffer.reset();
   m_PartialResultBuffer = make_shared<Buffer>(*m_CL, m_PartialResult.data(), BufferSize);
}

void Statistics::PrepareCoords(const ImageBase& Image)
{
   PrepareBuffer(Image);

   size_t NbGroups = (size_t) GetNbGroups(Image);

   // We are storing X and Y 
   size_t BufferSize = NbGroups * 2;

   if (m_PartialCoordBuffer != nullptr &&
      m_PartialCoordBuffer->Size() == BufferSize * sizeof(int) &&
      m_PartialCoord.size() == BufferSize)
   {
      return;
   }

   m_PartialCoord.assign(BufferSize, 0);

   m_PartialCoordBuffer.reset();
   m_PartialCoordBuffer = make_shared<Buffer>(*m_CL, m_PartialCoord.data(), BufferSize);
}


// Init
void Statistics::Init(IImage& Source)
{
   Source.SendIfNeeded();

   cl::make_kernel<cl::Image2D, cl::Buffer>(SelectProgram(Source), "init")
      (cl::EnqueueArgs(*m_CL, cl::NDRange(1)), Source, m_ResultBuffer);
}

void Statistics::InitAbs(IImage& Source)
{
   Source.SendIfNeeded();

   cl::make_kernel<cl::Image2D, cl::Buffer>(SelectProgram(Source), "init_abs")
      (cl::EnqueueArgs(*m_CL, cl::NDRange(1)), Source, m_ResultBuffer);
}

void Statistics::Init4C(IImage& Source)
{
   Source.SendIfNeeded();

   cl::make_kernel<cl::Image2D, cl::Buffer>(SelectProgram(Source), "init_4C")
      (cl::EnqueueArgs(*m_CL, cl::NDRange(1)), Source, m_ResultBuffer);
}

void Statistics::InitAbs4C(IImage& Source)
{
   Source.SendIfNeeded();

   cl::make_kernel<cl::Image2D, cl::Buffer>(SelectProgram(Source), "init_abs_4C")
      (cl::EnqueueArgs(*m_CL, cl::NDRange(1)), Source, m_ResultBuffer);
}


// Reductions on the first channel
double Statistics::Min(IImage& Source)
{
   Init(Source);

   Kernel(reduce_min, In(Source), Out(m_ResultBuffer), Source.Width(), Source.Height());

   m_ResultBuffer.Read(true);

   return m_Result[0];
}

double Statistics::Max(IImage& Source)
{
   Init(Source);

   Kernel(reduce_max, In(Source), Out(m_ResultBuffer), Source.Width(), Source.Height());

   m_ResultBuffer.Read(true);

   return m_Result[0];
}

double Statistics::MinAbs(IImage& Source)
{
   InitAbs(Source);

   Kernel(reduce_minabs, In(Source), Out(m_ResultBuffer), Source.Width(), Source.Height());

   m_ResultBuffer.Read(true);

   return m_Result[0];
}

double Statistics::MaxAbs(IImage& Source)
{
   InitAbs(Source);

   Kernel(reduce_maxabs, In(Source), Out(m_ResultBuffer), Source.Width(), Source.Height());

   m_ResultBuffer.Read(true);

   return m_Result[0];
}

double Statistics::Sum(IImage& Source)
{
   PrepareBuffer(Source);

   Kernel(reduce_sum, In(Source), Out(*m_PartialResultBuffer), Source.Width(), Source.Height());

   m_PartialResultBuffer->Read(true);

   return ReduceSum(m_PartialResult);
}

double Statistics::SumSqr(IImage& Source)
{
   PrepareBuffer(Source);

   Kernel(reduce_sum_sqr, In(Source), Out(*m_PartialResultBuffer), Source.Width(), Source.Height());

   m_PartialResultBuffer->Read(true);

   return ReduceSum(m_PartialResult);
}

uint Statistics::CountNonZero(IImage& Source)
{
   PrepareBuffer(Source);

   Kernel(reduce_count_nz, In(Source), Out(*m_PartialResultBuffer), Source.Width(), Source.Height());

   m_PartialResultBuffer->Read(true);

   return (uint) ReduceSum(m_PartialResult);
}

double Statistics::Mean(IImage& Source)
{
   PrepareBuffer(Source);

   Kernel(reduce_mean, In(Source), Out(*m_PartialResultBuffer), Source.Width(), Source.Height());

   m_PartialResultBuffer->Read(true);

   return ReduceMean(m_PartialResult);
}

double Statistics::MeanSqr(IImage& Source)
{
   PrepareBuffer(Source);

   Kernel(reduce_mean_sqr, In(Source), Out(*m_PartialResultBuffer), Source.Width(), Source.Height());

   m_PartialResultBuffer->Read(true);

   return ReduceMean(m_PartialResult);
}

// Reductions that also find the coordinate
double Statistics::Min(IImage& Source, int& outX, int& outY)
{
   PrepareCoords(Source);

   Kernel(min_coord, In(Source), Out(*m_PartialResultBuffer, *m_PartialCoordBuffer), Source.Width(), Source.Height());

   m_PartialResultBuffer->Read();
   m_PartialCoordBuffer->Read(true);

   return ReduceMin(m_PartialResult, m_PartialCoord, outX, outY);
}

double Statistics::Max(IImage& Source, int& outX, int& outY)
{
   PrepareCoords(Source);

   Kernel(max_coord, In(Source), Out(*m_PartialResultBuffer, *m_PartialCoordBuffer), Source.Width(), Source.Height());

   m_PartialResultBuffer->Read();
   m_PartialCoordBuffer->Read(true);

   return ReduceMax(m_PartialResult, m_PartialCoord, outX, outY);
}

double Statistics::MinAbs(IImage& Source, int& outX, int& outY)
{
   PrepareCoords(Source);

   Kernel(min_abs_coord, In(Source), Out(*m_PartialResultBuffer, *m_PartialCoordBuffer), Source.Width(), Source.Height());

   m_PartialResultBuffer->Read();
   m_PartialCoordBuffer->Read(true);

   return ReduceMin(m_PartialResult, m_PartialCoord, outX, outY);
}

double Statistics::MaxAbs(IImage& Source, int& outX, int& outY)
{
   PrepareCoords(Source);

   Kernel(max_abs_coord, In(Source), Out(*m_PartialResultBuffer, *m_PartialCoordBuffer), Source.Width(), Source.Height());

   m_PartialResultBuffer->Read();
   m_PartialCoordBuffer->Read(true);

   return ReduceMax(m_PartialResult, m_PartialCoord, outX, outY);
}


// For images with multiple channels
void Statistics::Min(IImage& Source, double outVal[4])
{
   Init4C(Source);

   Kernel(reduce_min_4C, In(Source), Out(m_ResultBuffer), Source.Width(), Source.Height());

   m_ResultBuffer.Read(true);

   for (uint i = 0; i < Source.NbChannels(); i++)
      outVal[i] = m_Result[i];
}

void Statistics::Max(IImage& Source, double outVal[4])
{
   Init4C(Source);

   Kernel(reduce_max_4C, In(Source), Out(m_ResultBuffer), Source.Width(), Source.Height());

   m_ResultBuffer.Read(true);

   for (uint i = 0; i < Source.NbChannels(); i++)
      outVal[i] = m_Result[i];
}

void Statistics::MinAbs(IImage& Source, double outVal[4])
{
   InitAbs4C(Source);

   Kernel(reduce_minabs_4C, In(Source), Out(m_ResultBuffer), Source.Width(), Source.Height());

   m_ResultBuffer.Read(true);

   for (uint i = 0; i < Source.NbChannels(); i++)
      outVal[i] = m_Result[i];
}

void Statistics::MaxAbs(IImage& Source, double outVal[4])
{
   InitAbs4C(Source);

   Kernel(reduce_maxabs_4C, In(Source), Out(m_ResultBuffer), Source.Width(), Source.Height());

   m_ResultBuffer.Read(true);

   for (uint i = 0; i < Source.NbChannels(); i++)
      outVal[i] = m_Result[i];
}

void Statistics::Sum(IImage& Source, double outVal[4])
{
   PrepareBuffer(Source);

   Kernel(reduce_sum_4C, In(Source), Out(*m_PartialResultBuffer), Source.Width(), Source.Height());

   m_PartialResultBuffer->Read(true);

   ReduceSum(m_PartialResult, Source.NbChannels(), outVal);
}

void Statistics::SumSqr(IImage& Source, double outVal[4])
{
   PrepareBuffer(Source);

   Kernel(reduce_sum_sqr_4C, In(Source), Out(*m_PartialResultBuffer), Source.Width(), Source.Height());

   m_PartialResultBuffer->Read(true);

   ReduceSum(m_PartialResult, Source.NbChannels(), outVal);
}

void Statistics::Mean(IImage& Source, double outVal[4])
{
   PrepareBuffer(Source);

   Kernel(reduce_mean_4C, In(Source), Out(*m_PartialResultBuffer), Source.Width(), Source.Height());

   m_PartialResultBuffer->Read(true);

   ReduceMean(m_PartialResult, Source.NbChannels(), outVal);
}

void Statistics::MeanSqr(IImage& Source, double outVal[4])
{
   PrepareBuffer(Source);

   Kernel(reduce_mean_sqr_4C, In(Source), Out(*m_PartialResultBuffer), Source.Width(), Source.Height());

   m_PartialResultBuffer->Read(true);

   ReduceMean(m_PartialResult, Source.NbChannels(), outVal);
}

}
