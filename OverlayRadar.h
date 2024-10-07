/*
MIT License

Copyright (c) 2021-2022 L. E. Spalt, Diego Schneider

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to w5m the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <array>
#include "Overlay.h"
#include "Config.h"
#include "OverlayDebug.h"

enum class radar_carLR {
    car_undefined,
    car_L,
    car_R,
};

class OverlayRadar : public Overlay
{
public:

    OverlayRadar(Microsoft::WRL::ComPtr<ID3D11Device> d3dDevice)
        : Overlay("OverlayRadar", d3dDevice)
    {}

protected:

    virtual float2 getDefaultSize()
    {
        return float2(400, 400);
    }

    virtual void onConfigChanged()
    {
        // Width might have changed, reset tracker values
    }

    virtual void onUpdate()
    {
        const float w = (float)m_width;
        const float h = (float)m_height;

        const float cornerRadius = g_cfg.getFloat(m_name, "corner_radius", 2.0f);
        const float markerWidth = g_cfg.getFloat(m_name, "marker_width", 20.0f);

        struct CarInfo {
            int     carIdx = 0;
            float   deltaMts = 0;
        };

        std::vector<CarInfo> radarInfo;
        radarInfo.reserve(IR_MAX_CARS);
        const float selfLapDistPct = ir_LapDistPct.getFloat();
        const float trackLength = ir_LapDist.getFloat() / selfLapDistPct;
        const float carLength = g_cfg.getFloat(m_name, "car_length", 5.0f);
        const float maxDist = g_cfg.getFloat(m_name, "maxDistance", 5.0f);

        for (int i = 0; i < IR_MAX_CARS; ++i)
        {
            const Car& car = ir_session.cars[i];
            const int lapcountCar = ir_CarIdxLap.getInt(i);

            if (lapcountCar >= 0 && !car.isSpectator && car.carNumber >= 0 && !car.isPaceCar && !ir_CarIdxOnPitRoad.getBool(i) )
            {
                const float carLapDistPct = ir_CarIdxLapDistPct.getFloat(i);
                const bool wrap = fabsf(selfLapDistPct - carLapDistPct) > 0.5f;

                float lapDistPctDelta = selfLapDistPct - carLapDistPct;
                
                if (wrap) {
                    if (selfLapDistPct > carLapDistPct) {
                        lapDistPctDelta -= 1;
                    }
                    else {
                        lapDistPctDelta += 1;
                    }
                }
                const float deltaMts = lapDistPctDelta * trackLength;
                radarInfo.emplace_back(i, deltaMts);

            }
        }

        std::sort(radarInfo.begin(), radarInfo.end(),
            [](const CarInfo& a, const CarInfo& b) {return a.deltaMts > b.deltaMts;});

        // Locate our driver's index in the new array, and nearest Ahead/Behind
        int selfCarInfoIdx = -1;
        int nearAhead, nearBehind;

        for (int i = 0; i < (int)radarInfo.size(); ++i)
        {
            if (radarInfo[i].carIdx == ir_session.driverCarIdx) {
                selfCarInfoIdx = i;

                if (i > 0) {
                    nearAhead = i - 1;
                }
                if (i + 1 <= (int)radarInfo.size()) {
                    nearBehind = i + 1;
                }
            }
        }
        // Something's wrong if we didn't find our driver. Bail.
        if (selfCarInfoIdx < 0)
            return;

        dbg("Radar: %.2f - Self: %.2f", trackLength, ir_LapDist.getFloat());
        

        D2D1_ROUNDED_RECT lRect, rRect;

        m_renderTarget->BeginDraw();
        for (const CarInfo ci : radarInfo) {

            if (fabsf(ci.deltaMts) > maxDist || ci.deltaMts == 0)
                continue;
            /* 
                top = 0 when
                    deltaMts = maxDist
                top = 1 when
                    deltaMts = -maxDist
            */

            const float rect_top = min(max(ci.deltaMts, -maxDist), maxDist) / maxDist;
            const float rect_bot = min(max(ci.deltaMts+carLength, -maxDist), maxDist) / maxDist;
            dbg("Deltamts: %f", ci.deltaMts); 

            // Left side
            lRect.rect = { 0, h * rect_top, markerWidth, h * rect_bot };
            lRect.radiusX = cornerRadius;
            lRect.radiusY = cornerRadius;

            // Right side
            rRect.rect = { w - markerWidth, h * rect_top, w, h * rect_bot };
            rRect.radiusX = cornerRadius;
            rRect.radiusY = cornerRadius;

            const int carLeftRight = ir_CarLeftRight.getInt();

            if (carLeftRight == irsdk_LRClear) {
                m_brush->SetColor(g_cfg.getFloat4(m_name, "car_far_fill_col", float4(1.0f, 1.0f, 0.0f, 0.5f)));
                m_renderTarget->FillRoundedRectangle(&lRect, m_brush.Get());
                m_renderTarget->FillRoundedRectangle(&rRect, m_brush.Get());
            }
            else {
                m_brush->SetColor(g_cfg.getFloat4(m_name, "car_near_fill_col", float4(1.0f, 0.2f, 0.0f, 0.5f)));

                switch (carLeftRight) {
                    case irsdk_LRCarLeft:
                    case irsdk_LR2CarsLeft:
                    case irsdk_LRCarLeftRight:
                        m_renderTarget->FillRoundedRectangle(&lRect, m_brush.Get());
                }

                switch (carLeftRight) {
                    case irsdk_LRCarRight:
                    case irsdk_LR2CarsRight:
                    case irsdk_LRCarLeftRight:
                        m_renderTarget->FillRoundedRectangle(&rRect, m_brush.Get());
                }
            }
        
            /*
            switch (carLeftRight)
            {
            case irsdk_LRClear:
                

            case irsdk_LRCarLeft:
            case irsdk_LR2CarsLeft:
                m_renderTarget->FillRoundedRectangle(&lRect, m_brush.Get());
                break;
            case irsdk_LRCarRight:
            case irsdk_LR2CarsRight:
                m_renderTarget->FillRoundedRectangle(&rRect, m_brush.Get());
                break;

            case irsdk_LRCarLeftRight:
                m_renderTarget->FillRoundedRectangle(&lRect, m_brush.Get());
                m_renderTarget->FillRoundedRectangle(&rRect, m_brush.Get());
                break;
            }
            */
        }
        
        m_renderTarget->EndDraw();
    }

protected:

    std::array<int, IR_MAX_CARS> m_persistentLR;
};
