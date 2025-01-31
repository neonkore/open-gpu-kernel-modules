/*
 * SPDX-FileCopyrightText: Copyright (c) 2010-2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/******************************* List **************************************\
*                                                                           *
* Module: dp_linkconfig.h                                                   *
*   Link Configuration object implementation                                *
*                                                                           *
\***************************************************************************/
#ifndef INCLUDED_DP_LINKCONFIG_H
#define INCLUDED_DP_LINKCONFIG_H

#include "dp_auxdefs.h"
#include "dp_internal.h"
#include "dp_watermark.h"
#include "ctrl/ctrl0073/ctrl0073specific.h" // NV0073_CTRL_HDCP_VPRIME_SIZE
#include "displayport.h"

namespace DisplayPort
{
    typedef NvU64 LinkRate;

    class LinkRates : virtual public Object
    {
    public:
        NvU8 entries;

        virtual void clear() = 0;
        virtual bool import(NvU8 linkBw)
        {
            DP_ASSERT(0);
            return false;
        }

        virtual LinkRate getLowerRate(LinkRate rate) = 0;
        virtual LinkRate getMaxRate() = 0;
        virtual NvU8 getNumElements() = 0;

        NvU8 getNumLinkRates()
        {
            return entries;
        }

    };

    class LinkRates1x : virtual public LinkRates
    {
    public:
        // Store link rate in multipler of 270MBPS to save space
        NvU8 element[NV_SUPPORTED_DP1X_LINK_RATES__SIZE];

        LinkRates1x()
        {
            entries = 0;
            for (int i = 0; i < NV_SUPPORTED_DP1X_LINK_RATES__SIZE; i++)
            {
                element[i] = 0;
            }
        }

        virtual void clear()
        {
            entries = 0;
            for (int i = 0; i < NV_SUPPORTED_DP1X_LINK_RATES__SIZE; i++)
            {
                element[i] = 0;
            }
        }

        virtual bool import(NvU8 linkBw)
        {
            if (!IS_VALID_LINKBW(linkBw))
            {
                DP_ASSERT(0 && "Unsupported Link Bandwidth");
                return false;
            }

            if (entries < NV_SUPPORTED_DP1X_LINK_RATES__SIZE)
            {
                element[entries] = linkBw;
                entries++;
                return true;
            }
            else
                return false;
        }

        virtual LinkRate getLowerRate(LinkRate rate)
        {
            int i;
            NvU8 linkBw = (NvU8)(rate / DP_LINK_BW_FREQ_MULTI_MBPS);

            if ((entries == 0) || (linkBw <= element[0]))
                return 0;

            for (i = entries - 1; i > 0; i--)
            {
                if (linkBw > element[i])
                    break;
            }

            rate = (LinkRate)element[i] * DP_LINK_BW_FREQ_MULTI_MBPS;
            return rate;
        }

        virtual LinkRate getMaxRate()
        {
            LinkRate rate = 0;
            if ((entries > 0) &&
                (entries <= NV_SUPPORTED_DP1X_LINK_RATES__SIZE))
            {
                rate = (LinkRate)element[entries - 1] * DP_LINK_BW_FREQ_MULTI_MBPS;
            }

            return rate;
        }
        virtual NvU8 getNumElements()
        {
            return NV_SUPPORTED_DP1X_LINK_RATES__SIZE;
        }

    };

    class LinkPolicy : virtual public Object
    {
    protected:
        bool            bNoFallback;                // No fallback when LT fails
        LinkRates1x     linkRates;
    public:
        LinkPolicy() : bNoFallback(false)
        {
        }
        bool skipFallback()
        {
            return bNoFallback;
        }
        void setSkipFallBack(bool bSkipFallback)
        {
            bNoFallback = bSkipFallback;
        }

        LinkRates *getLinkRates()
        {
            return &linkRates;
        }
    };

    enum
    {
        totalTimeslots = 64,
        totalUsableTimeslots = totalTimeslots - 1
    };

    //
    // Link Data Rate per DP Lane, in MBPS,
    // For 8b/10b channel coding:
    //     Link Data Rate = link rate * (8 / 10) / 8
    //                    = link rate * 0.1
    // For 128b/132b channel coding:
    //     Link Data Rate = link rate * (128 / 132) / 8
    //                    = link rate * 4 / 33
    //                   ~= link rate * 0.12
    //
    // Link Bandwidth     = Lane Count * Link Data Rate
    //
    enum
    {
        RBR             =  162000000,
        EDP_2_16GHZ     =  216000000,
        EDP_2_43GHZ     =  243000000,
        HBR             =  270000000,
        EDP_3_24GHZ     =  324000000,
        EDP_4_32GHZ     =  432000000,
        HBR2            =  540000000,
        HBR3            =  810000000
    };

    struct HDCPState
    {
        bool HDCP_State_Encryption;
        bool HDCP_State_1X_Capable;
        bool HDCP_State_22_Capable;
        bool HDCP_State_Authenticated;
        bool HDCP_State_Repeater_Capable;
    };

    struct HDCPValidateData
    {
    };

    typedef enum
    {
        DP_SINGLE_HEAD_MULTI_STREAM_MODE_NONE,
        DP_SINGLE_HEAD_MULTI_STREAM_MODE_SST,
        DP_SINGLE_HEAD_MULTI_STREAM_MODE_MST,
    }DP_SINGLE_HEAD_MULTI_STREAM_MODE;

#define HEAD_INVALID_STREAMS 0
#define HEAD_DEFAULT_STREAMS 1

    typedef enum
    {
        DP_SINGLE_HEAD_MULTI_STREAM_PIPELINE_ID_PRIMARY     = 0,
        DP_SINGLE_HEAD_MULTI_STREAM_PIPELINE_ID_SECONDARY   = 1,
        DP_SINGLE_HEAD_MULTI_STREAM_PIPELINE_ID_MAX         = DP_SINGLE_HEAD_MULTI_STREAM_PIPELINE_ID_SECONDARY,
    } DP_SINGLE_HEAD_MULTI_STREAM_PIPELINE_ID;

#define DP_INVALID_SOR_INDEX 0xFFFFFFFF
#define DSC_DEPTH_FACTOR     16


    class LinkConfiguration : virtual public Object
    {
    public:
        LinkPolicy policy;
        unsigned lanes;
        LinkRate peakRatePossible;
        LinkRate peakRate;
        LinkRate minRate;
        bool     enhancedFraming;
        bool     multistream;
        bool     disablePostLTRequest;
        bool     bEnableFEC;
        bool     bDisableLTTPR;
        //
        // The counter to record how many times link training happens.
        // Client can reset the counter by calling setLTCounter(0)
        //
        unsigned linkTrainCounter;

        LinkConfiguration() :
            lanes(0), peakRatePossible(0), peakRate(0), minRate(0),
            enhancedFraming(false), multistream(false), disablePostLTRequest(false),
            bEnableFEC(false), bDisableLTTPR(false),
            linkTrainCounter(0) {};

        LinkConfiguration(LinkPolicy * p, unsigned lanes, LinkRate peakRate,
            bool enhancedFraming, bool MST, bool disablePostLTRequest = false,
            bool bEnableFEC = false, bool bDisableLTTPR = false) :
                lanes(lanes), peakRatePossible(peakRate), peakRate(peakRate),
                enhancedFraming(enhancedFraming), multistream(MST),
                disablePostLTRequest(disablePostLTRequest),
                bEnableFEC(bEnableFEC), bDisableLTTPR(bDisableLTTPR),
                linkTrainCounter(0)
        {
            // downrate for spread and FEC
            minRate = linkOverhead(peakRate);
            if (p)
            {
                policy = *p;
            }
        }

        void setLTCounter(unsigned counter)
        {
            linkTrainCounter = counter;
        }

        unsigned getLTCounter()
        {
            return linkTrainCounter;
        }

        NvU64 linkOverhead(NvU64 rate)
        {
            if(bEnableFEC)
            {

                // if FEC is enabled, we have to account for 3% overhead
                // for FEC+downspread according to DP 1.4 spec

                return rate - 3 * rate/ 100;
            }
            else
            {
                // if FEC is not enabled, link overhead comprises only of
                // 0.05% downspread.
                return rate - 5 * rate/ 1000;

            }
        }

        void enableFEC(bool setFEC)
        {
            bEnableFEC = setFEC;

            // If FEC is enabled, update minRate with FEC+downspread overhead.
            minRate = linkOverhead(peakRate);
        }

        LinkConfiguration(unsigned long TotalLinkPBN)
            : enhancedFraming(true),
              multistream(true),
              disablePostLTRequest(false),
              bEnableFEC(false),
              bDisableLTTPR(false),
              linkTrainCounter(0)
        {
            //
            // Reverse engineer a link configuration from Total TotalLinkPBN
            // Note that HBR2 twice HBR. The table below treats HBR2x1 and HBRx2, etc.
            //
            // PBN Calculation
            // Definition of PBN is "54/64 MBps".
            // Note this is the "data" actually transmitted in the main link.
            // So we need to take channel coding into consideration.
            // Formula: PBN = Lane Count * Link Rate (Gbps) * 1000 * (1/8) * ChannelCoding Efficiency * (64 / 54)
            // Example:
            // 1. 4 * HBR2:    4 * 5.4 * 1000 * (1/8) * (8/10) * (64/54) = 2560
            // 2. 2 * UHBR10:  2 * 10 * 1000 * (1/8) * (128/132) * (64/54) = 2873
            //
            // Full list:
            //
            //   BW (Gbps)        Lanes              TotalLinkPBN
            //     1.62             1                     192
            //     1.62             2                     384
            //     1.62             4                     768
            //     2.70             1                     320
            //     2.70             2                     640
            //     2.70             4                    1280
            //     5.40             1                     640
            //     5.40             2                    1280
            //     5.40             4                    2560
            //     8.10             1                     960
            //     8.10             2                    1920
            //     8.10             4                    3840
            //    10.00             1                    1436
            //    10.00             2                    2873
            //    10.00             4                    5746
            //    13.50             1                    1939
            //    13.50             2                    3878
            //    13.50             4                    7757
            //    20.00             1                    2873
            //    20.00             2                    5746
            //    20.00             4                   11492
            //

            if (TotalLinkPBN <= 90)
            {
                peakRatePossible = peakRate = RBR;
                minRate = linkOverhead(RBR);
                lanes = 0; // FAIL
            }
            if (TotalLinkPBN <= 192)
            {
                peakRatePossible = peakRate = RBR;
                minRate = linkOverhead(RBR);
                lanes = 1;
            }
            else if (TotalLinkPBN <= 320)
            {
                peakRatePossible = peakRate = HBR;
                minRate = linkOverhead(HBR);
                lanes = 1;
            }
            else if (TotalLinkPBN <= 384)
            {
                peakRatePossible = peakRate = RBR;
                minRate = linkOverhead(RBR);
                lanes = 2;
            }
            else if (TotalLinkPBN <= 640)
            {
                // could be HBR2 x 1, but TotalLinkPBN works out same
                peakRatePossible = peakRate = HBR;
                minRate = linkOverhead(HBR);
                lanes = 2;
            }
            else if (TotalLinkPBN <= 768)
            {
                peakRatePossible = peakRate = RBR;
                minRate = linkOverhead(RBR);
                lanes = 4;
            }
            else if (TotalLinkPBN <= 960)
            {
                peakRatePossible = peakRate = HBR3;
                minRate = linkOverhead(HBR3);
                lanes = 1;
            }
            else if (TotalLinkPBN <= 1280)
            {
                // could be HBR2 x 2
                peakRatePossible = peakRate = HBR;
                minRate = linkOverhead(HBR);
                lanes = 4;
            }
            else if (TotalLinkPBN <= 1920)
            {
                peakRatePossible = peakRate = HBR3;
                minRate = linkOverhead(HBR3);
                lanes = 2;
            }
            else if (TotalLinkPBN <= 2560)
            {
                peakRatePossible = peakRate = HBR2;
                minRate = linkOverhead(HBR2);
                lanes = 4;
            }
            else if (TotalLinkPBN <= 3840)
            {
                peakRatePossible = peakRate = HBR3;
                minRate = linkOverhead(HBR3);
                lanes = 4;
            }
            else {
                peakRatePossible = peakRate = RBR, minRate = linkOverhead(RBR), lanes = 0; // FAIL
                DP_ASSERT(0 && "Unknown configuration");
            }
        }

        void setEnhancedFraming(bool newEnhancedFraming)
        {
            enhancedFraming = newEnhancedFraming;
        }

        bool isValid()
        {
            return lanes != laneCount_0;
        }

        bool lowerConfig(bool bReduceLaneCnt = false)
        {
            //
            // TODO: bReduceLaneCnt is set to fallback to 4 lanes with lower
            //       valid link rate. But we should reset to max lane count
            //       sink supports instead.
            //

            LinkRate lowerRate = policy.getLinkRates()->getLowerRate(peakRate);

            if(bReduceLaneCnt)
            {
                // Reduce laneCount before reducing linkRate
                if(lanes == laneCount_1)
                {
                    if (lowerRate)
                    {
                        lanes = laneCount_4;
                        peakRate = lowerRate;
                    }
                    else
                    {
                        lanes = laneCount_0;
                    }
                }
                else
                {
                    lanes /= 2;
                }
            }
            else
            {
                // Reduce the link rate instead of lane count
                if (lowerRate)
                {
                    peakRate = lowerRate;
                }
                else
                {
                    lanes /= 2;
                }
            }

            minRate = linkOverhead(peakRate);
            return lanes != laneCount_0;
        }

        void setLaneRate(LinkRate newRate, unsigned newLanes)
        {
            peakRate  = newRate;
            lanes = newLanes;
            minRate = linkOverhead(peakRate);
        }

        unsigned pbnTotal()
        {
            return PBNForSlots(totalUsableTimeslots);
        }

        void pbnRequired(const ModesetInfo & modesetInfo, unsigned & base_pbn, unsigned & slots, unsigned & slots_pbn)
        {
            base_pbn = pbnForMode(modesetInfo);
            if (bEnableFEC)
            {
                // IF FEC is enabled, we need to consider 3% overhead as per DP1.4 spec.
                base_pbn = (NvU32)(divide_ceil(base_pbn * 100, 97));
            }
            slots = slotsForPBN(base_pbn);
            slots_pbn = PBNForSlots(slots);
        }

        NvU32 slotsForPBN(NvU32 allocatedPBN, bool usable = false)
        {
            NvU64 bytes_per_pbn      = 54 * 1000000 / 64;     // this comes out exact
            NvU64 bytes_per_timeslot = peakRate * lanes / 64;

            if (bytes_per_timeslot == 0)
                return (NvU32)-1;

            if (usable)
            {
                // round down to find the usable integral slots for a given value of PBN.
                NvU32 slots = (NvU32)divide_floor(allocatedPBN * bytes_per_pbn, bytes_per_timeslot);
                DP_ASSERT(slots <= 64);

                return slots;
            }
            else
                return (NvU32)divide_ceil(allocatedPBN * bytes_per_pbn, bytes_per_timeslot);
        }

        NvU32 PBNForSlots(NvU32 slots)                      // Rounded down
        {
            NvU64 bytes_per_pbn      = 54 * 1000000 / 64;     // this comes out exact
            NvU64 bytes_per_timeslot = peakRate * lanes / 64;

            return (NvU32)(bytes_per_timeslot * slots/ bytes_per_pbn);
        }

        bool operator!= (const LinkConfiguration & right) const
        {
            return !(*this == right);
        }

        bool operator== (const LinkConfiguration & right) const
        {
            return (this->lanes == right.lanes &&
                    this->peakRate == right.peakRate &&
                    this->enhancedFraming == right.enhancedFraming &&
                    this->multistream == right.multistream &&
                    this->bEnableFEC == right.bEnableFEC);
        }

        bool operator< (const LinkConfiguration & right) const
        {
            NvU64 leftMKBps = peakRate * lanes;
            NvU64 rightMKBps = right.peakRate * right.lanes;

            if (leftMKBps == rightMKBps)
            {
                return (lanes < right.lanes);
            }
            else
            {
                return (leftMKBps < rightMKBps);
            }
        }
    };
}
#endif //INCLUDED_DP_LINKCONFIG_H
