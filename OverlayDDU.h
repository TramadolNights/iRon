/*
MIT License

Copyright (c) 2021-2022 L. E. Spalt

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
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

#include <vector>
#include <algorithm>
#include <deque>
#include "Overlay.h"
#include "iracing.h"
#include "Config.h"
#include "OverlayDebug.h"

class OverlayDDU : public Overlay
{
    public:

        const float DefaultFontSize = 17;

        OverlayDDU(Microsoft::WRL::ComPtr<ID3D11Device> d3dDevice)
            : Overlay("OverlayDDU", d3dDevice)
        {}

       #ifdef _DEBUG
       virtual bool    canEnableWhileNotDriving() const { return true; }
       virtual bool    canEnableWhileDisconnected() const { return true; }
       #endif


    protected:

        struct Box
        {
            float x0 = 0;
            float x1 = 0;
            float y0 = 0;
            float y1 = 0;
            float w = 0;
            float h = 0;
            std::string title;
        };

        virtual float2 getDefaultSize()
        {
            return float2(809,166);
        }

        virtual void onEnable()
        {
            onConfigChanged();
        }

        virtual void onDisable()
        {
            m_text.reset();
        }

        virtual void onConfigChanged()
        {
            // Font stuff
            {
                m_text.reset( m_dwriteFactory.Get() );

                const std::string font = g_cfg.getString( m_name, "font", "Arial" );
                const float fontSize = g_cfg.getFloat( m_name, "font_size", DefaultFontSize );
                const int fontWeight = g_cfg.getInt( m_name, "font_weight", 500 );
                HRCHECK(m_dwriteFactory->CreateTextFormat( toWide(font).c_str(), NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fontSize, L"en-us", &m_textFormat ));
                m_textFormat->SetParagraphAlignment( DWRITE_PARAGRAPH_ALIGNMENT_CENTER );
                m_textFormat->SetWordWrapping( DWRITE_WORD_WRAPPING_NO_WRAP );

                HRCHECK(m_dwriteFactory->CreateTextFormat( toWide(font).c_str(), NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fontSize, L"en-us", &m_textFormatBold ));
                m_textFormatBold->SetParagraphAlignment( DWRITE_PARAGRAPH_ALIGNMENT_CENTER );
                m_textFormatBold->SetWordWrapping( DWRITE_WORD_WRAPPING_NO_WRAP );

                HRCHECK(m_dwriteFactory->CreateTextFormat( toWide(font).c_str(), NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fontSize*1.2f, L"en-us", &m_textFormatLarge ));
                m_textFormatLarge->SetParagraphAlignment( DWRITE_PARAGRAPH_ALIGNMENT_CENTER );
                m_textFormatLarge->SetWordWrapping( DWRITE_WORD_WRAPPING_NO_WRAP );

                HRCHECK(m_dwriteFactory->CreateTextFormat( toWide(font).c_str(), NULL, DWRITE_FONT_WEIGHT_LIGHT, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fontSize*0.7f, L"en-us", &m_textFormatSmall ));
                m_textFormatSmall->SetParagraphAlignment( DWRITE_PARAGRAPH_ALIGNMENT_CENTER );
                m_textFormatSmall->SetWordWrapping( DWRITE_WORD_WRAPPING_NO_WRAP );

                HRCHECK(m_dwriteFactory->CreateTextFormat( toWide(font).c_str(), NULL, DWRITE_FONT_WEIGHT_LIGHT, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fontSize*0.6f, L"en-us", &m_textFormatVerySmall ));
                m_textFormatVerySmall->SetParagraphAlignment( DWRITE_PARAGRAPH_ALIGNMENT_CENTER );
                m_textFormatVerySmall->SetWordWrapping( DWRITE_WORD_WRAPPING_NO_WRAP );

                HRCHECK(m_dwriteFactory->CreateTextFormat( toWide(font).c_str(), NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fontSize*3.0f, L"en-us", &m_textFormatGear ));
                m_textFormatGear->SetParagraphAlignment( DWRITE_PARAGRAPH_ALIGNMENT_CENTER );
                m_textFormatGear->SetWordWrapping( DWRITE_WORD_WRAPPING_NO_WRAP );
            }

            // Background geometry
            {
                Microsoft::WRL::ComPtr<ID2D1GeometrySink>  geometrySink;
                m_d2dFactory->CreatePathGeometry( &m_backgroundPathGeometry );
                m_backgroundPathGeometry->Open( &geometrySink );

                const float w = (float)m_width;
                const float h = (float)m_height;

                geometrySink->BeginFigure( float2(0,h), D2D1_FIGURE_BEGIN_FILLED );
                geometrySink->AddBezier( D2D1::BezierSegment(float2(0,-h/3),float2(w,-h/3),float2(w,h)) );
                geometrySink->EndFigure( D2D1_FIGURE_END_CLOSED );

                geometrySink->Close();
            }

            // Box geometries
            {
                Microsoft::WRL::ComPtr<ID2D1GeometrySink>  geometrySink;
                m_d2dFactory->CreatePathGeometry( &m_boxPathGeometry );
                m_boxPathGeometry->Open( &geometrySink );

                const float vtop = 0.13f;
                const float hgap = 0.005f;
                const float vgap = 0.05f;
                const float gearw = 0.09f;
                const float w1 = 0.06f;
                const float w2 = 0.12f;
                const float h1 = 0.24f;
                const float h2 = 2*h1+vgap;
                const float h3 = 3*h1+2*vgap;
            
                m_boxGear = makeBox( 0.5f-gearw/2, gearw, vtop, 0.53f, "" );
                addBoxFigure( geometrySink.Get(), m_boxGear );

                m_boxDelta = makeBox( 0.5f-gearw/2, gearw, vtop+2*vgap+2*h1, h1, "vs Best" );
                addBoxFigure( geometrySink.Get(), m_boxDelta );
            
                m_boxBest = makeBox( 0.5f-gearw/2-hgap-w2, w2, vtop, h1, "Best" );
                addBoxFigure( geometrySink.Get(), m_boxBest );
            
                m_boxLast = makeBox( 0.5f-gearw/2-hgap-w2, w2, vtop+vgap+h1, h1, "Last" );
                addBoxFigure( geometrySink.Get(), m_boxLast );

                m_boxP1Last = makeBox( 0.5f-gearw/2-hgap-w2, w2, vtop+2*vgap+2*h1, h1, "P1 Last" );
                addBoxFigure( geometrySink.Get(), m_boxP1Last );

                m_boxLaps = makeBox( 0.5f-gearw/2-2*hgap-2*w2, w2, vtop+vgap+h1, h2, "Lap" );
                addBoxFigure( geometrySink.Get(), m_boxLaps );

                m_boxSession = makeBox( 0.5f-gearw/2-2*hgap-2*w2, w2, vtop+h1/3, h1*2.f/3.f, "Session" );
                addBoxFigure( geometrySink.Get(), m_boxSession );

                m_boxPos = makeBox( 0.5f-gearw/2-3*hgap-2*w2-w1, w1, vtop+vgap+h1, h1, "Pos" );
                addBoxFigure( geometrySink.Get(), m_boxPos );

                m_boxLapDelta = makeBox( 0.5f-gearw/2-3*hgap-2*w2-w1, w1, vtop+2*vgap+2*h1, h1, "Lap " );
                addBoxFigure( geometrySink.Get(), m_boxLapDelta );

                m_boxInc = makeBox( 0.5f-gearw/2-4*hgap-2*w2-2*w1, w1, vtop+2*vgap+2*h1, h1, "Inc" );
                addBoxFigure( geometrySink.Get(), m_boxInc );

                m_boxFuel = makeBox( 0.5f+gearw/2+hgap, w2, vtop, h3, "Fuel" );
                addBoxFigure( geometrySink.Get(), m_boxFuel );

                m_boxBias = makeBox( 0.5f+gearw/2+4*(hgap*1.4)+2*w2+1*w1, w1, vtop+2*vgap+2*h1, h1, "Bias" );
                addBoxFigure( geometrySink.Get(), m_boxBias );
            
                m_boxTires = makeBox( 0.5f+gearw/2+2*(hgap*1.25)+w2, w2, vtop+2*vgap+2*h1, h1, "Tires" );
                addBoxFigure( geometrySink.Get(), m_boxTires );

                m_boxOil = makeBox( 0.5f+gearw/2+2*hgap+w2, w1, vtop+vgap+h1, h1, "Oil" );
                addBoxFigure( geometrySink.Get(), m_boxOil );

                m_boxWater = makeBox( 0.5f+gearw/2+3*hgap+w2+w1, w1, vtop+vgap+h1, h1, "Wat" );
                addBoxFigure( geometrySink.Get(), m_boxWater );

                m_boxABS = makeBox( 0.5f+gearw/2+3*(hgap*1.4)+2*w2, w1, vtop+vgap+h1, h1, "ABS" );
                addBoxFigure(geometrySink.Get(), m_boxABS);

				m_boxTC = makeBox( 0.5f+gearw/2+3*(hgap*1.4)+2*w2, w1, vtop+2*vgap+2*h1, h1, "TC" );
				addBoxFigure(geometrySink.Get(), m_boxTC);
                
                geometrySink->Close();
            }

            // Static background cache
            Microsoft::WRL::ComPtr<ID2D1BitmapRenderTarget> bmpTarget;
            m_renderTarget->CreateCompatibleRenderTarget(&bmpTarget);
            bmpTarget->BeginDraw();
            bmpTarget->Clear();
            
            // Draw the background
            m_brush->SetColor( g_cfg.getFloat4( m_name, "background_col", float4(0,0,0,0.5f) ) );
            bmpTarget->FillGeometry( m_backgroundPathGeometry.Get(), m_brush.Get() );

            // Draw the boxes and static texts
            // FIXME: Move outlineCol and other variables to m_cfg_outlineCol so we don't declare TWO default values?
            const float4 outlineCol         = g_cfg.getFloat4( m_name, "outline_col", float4(0.7f,0.7f,0.7f,0.9f) );
            m_brush->SetColor( outlineCol );
            bmpTarget->DrawGeometry( m_boxPathGeometry.Get(), m_brush.Get() );
            m_text.render( bmpTarget.Get(), L"Lap",     m_textFormatSmall.Get(), m_boxLaps.x0, m_boxLaps.x1, m_boxLaps.y0, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER );
            m_text.render( bmpTarget.Get(), L"Pos",     m_textFormatSmall.Get(), m_boxPos.x0, m_boxPos.x1, m_boxPos.y0, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER );
            m_text.render( bmpTarget.Get(), L"Lap \u0394",m_textFormatSmall.Get(), m_boxLapDelta.x0, m_boxLapDelta.x1, m_boxLapDelta.y0, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER );
            m_text.render( bmpTarget.Get(), L"Best",    m_textFormatSmall.Get(), m_boxBest.x0, m_boxBest.x1, m_boxBest.y0, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER );
            m_text.render( bmpTarget.Get(), L"Last",    m_textFormatSmall.Get(), m_boxLast.x0, m_boxLast.x1, m_boxLast.y0, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER );
            m_text.render( bmpTarget.Get(), L"P1 Last", m_textFormatSmall.Get(), m_boxP1Last.x0, m_boxP1Last.x1, m_boxP1Last.y0, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER );
            m_text.render( bmpTarget.Get(), L"Fuel",    m_textFormatSmall.Get(), m_boxFuel.x0, m_boxFuel.x1, m_boxFuel.y0, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER );
            m_text.render( bmpTarget.Get(), L"Tires",   m_textFormatSmall.Get(), m_boxTires.x0, m_boxTires.x1, m_boxTires.y0, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER );
            m_text.render( bmpTarget.Get(), L"vs Best", m_textFormatSmall.Get(), m_boxDelta.x0, m_boxDelta.x1, m_boxDelta.y0, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER );
            m_text.render( bmpTarget.Get(), L"Session", m_textFormatSmall.Get(), m_boxSession.x0, m_boxSession.x1, m_boxSession.y0, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER );
            m_text.render( bmpTarget.Get(), L"Bias",    m_textFormatSmall.Get(), m_boxBias.x0, m_boxBias.x1, m_boxBias.y0, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER );
            m_text.render( bmpTarget.Get(), L"Inc",     m_textFormatSmall.Get(), m_boxInc.x0, m_boxInc.x1, m_boxInc.y0, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER );
            m_text.render( bmpTarget.Get(), L"Oil",     m_textFormatSmall.Get(), m_boxOil.x0, m_boxOil.x1, m_boxOil.y0, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER );
            m_text.render( bmpTarget.Get(), L"Water",   m_textFormatSmall.Get(), m_boxWater.x0, m_boxWater.x1, m_boxWater.y0, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER );
			m_text.render( bmpTarget.Get(), L"ABS",     m_textFormatSmall.Get(), m_boxABS.x0, m_boxABS.x1, m_boxABS.y0, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER );
			m_text.render( bmpTarget.Get(), L"TC",      m_textFormatSmall.Get(), m_boxTC.x0, m_boxTC.x1, m_boxTC.y0, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER );
            
            bmpTarget->EndDraw();
            bmpTarget->GetBitmap(&m_backgroundBitmap);
            // Delete or release m_BmpTarget?
            //bmpTarget->Release();
        }

        virtual void onSessionChanged()
        {
            m_isValidFuelLap = false;  // avoid confusing the fuel calculator logic with session changes
        }

        virtual void onUpdate()
        {
            const float  fontSize           = g_cfg.getFloat( m_name, "font_size", DefaultFontSize );
            const float4 outlineCol         = g_cfg.getFloat4( m_name, "outline_col", float4(0.7f,0.7f,0.7f,0.9f) );
            const float4 textCol            = g_cfg.getFloat4( m_name, "text_col", float4(1,1,1,0.9f) );
            const float4 goodCol            = g_cfg.getFloat4( m_name, "good_col", float4(0,0.8f,0,0.6f) );
            const float4 badCol             = g_cfg.getFloat4( m_name, "bad_col", float4(0.8f,0.1f,0.1f,0.6f) );
            const float4 fastestCol         = g_cfg.getFloat4( m_name, "fastest_col", float4(0.8f,0,0.8f,0.6f) );
            const float4 serviceCol         = g_cfg.getFloat4( m_name, "service_col", float4(0.36f,0.61f,0.84f,1) );
            const float4 warnCol            = g_cfg.getFloat4( m_name, "warn_col", float4(1,0.6f,0,1) );
            const float4 shiftCol           = g_cfg.getFloat4( m_name, "shift_col", float4(1, 0.1f, 0.1f, 0.6f) );
            const float4 pitCol             = g_cfg.getFloat4( m_name, "pit_col", float4(0, 0.8f, 0, 0.6f) );

            const int  carIdx   = ir_session.driverCarIdx;
            const bool imperial = ir_DisplayUnits.getInt() == 0;

            const string speedUnit = g_cfg.getString(m_name, "speed_unit", imperial==true ? "imperial" : "metric");

            const DWORD tickCount = GetTickCount64();

            // Figure out who's P1
            int p1carIdx = -1;
            for( int i=0; i<IR_MAX_CARS; ++i )
            {
                if( ir_getPosition(i) == 1 ) {
                    p1carIdx = i;
                    break;
                }
            }

            // General lap info
            const bool   sessionIsTimeLimited  = ir_SessionLapsTotal.getInt() == 32767 && ir_SessionTimeRemain.getDouble()<48.0*3600.0;  // most robust way I could find to figure out whether this is a time-limited session (info in session string is often misleading)
            const double remainingSessionTime  = sessionIsTimeLimited ? ir_SessionTimeRemain.getDouble() : -1;
            const int    remainingLaps         = sessionIsTimeLimited ? int(0.5+remainingSessionTime/ir_estimateLaptime()) : (ir_SessionLapsRemainEx.getInt() != 32767 ? ir_SessionLapsRemainEx.getInt() : -1);
            const int    targetLap             = g_cfg.getInt(m_name, "fuel_target_lap", 0);
            const int    currentLap            = ir_isPreStart() ? 0 : std::max(0,ir_CarIdxLap.getInt(carIdx));
            const bool   lapCountUpdated       = currentLap != m_prevCurrentLap;
            m_prevCurrentLap = currentLap;
            if( lapCountUpdated )
                m_lastLapChangeTickCount = tickCount;

            dbg( "isUnlimitedTime: %d, isUnlimitedLaps: %d, rem laps: %d, total laps: %d, rem time: %f", (int)ir_session.isUnlimitedTime, (int)ir_session.isUnlimitedLaps, ir_SessionLapsRemainEx.getInt(), ir_SessionLapsTotal.getInt(), ir_SessionTimeRemain.getFloat() );

            wchar_t s[512];

            m_renderTarget->BeginDraw();
            m_brush->SetColor( textCol );

            // Render the cached background
            {
                m_renderTarget->Clear( float4(0,0,0,0) );
                m_renderTarget->DrawBitmap(m_backgroundBitmap.Get());
            }

            // RPM lights
            {
                // which of the rpm numbers to use for high/low and colored light indicators was a bit of
                // trial and error, since I'm not really sure what they're supposed to mean exactly
                const float lo  = (ir_session.rpmIdle + ir_session.rpmSLFirst) / 2;
                const float hi  = ir_session.rpmRedline;
                const float rpm = ir_RPM.getFloat();
                const float rpmPct = (rpm-lo) / (hi-lo);

                const float ww = 0.16f;
                for( int i=0; i<8; ++i )
                {
                    const float lightPct = i/8.0f;
                    const float lightRpm = lo + (hi-lo) * lightPct;

                    D2D1_ELLIPSE e = { float2(r2ax(0.5f-ww/2+(i+0.5f)*ww/8),r2ay(0.065f)), r2ax(0.007f), r2ax(0.007f) };

                    if( rpmPct < lightPct ) {
                        m_brush->SetColor( outlineCol );
                        m_renderTarget->DrawEllipse( &e, m_brush.Get() );
                    }
                    else {
                        if( lightRpm < ir_session.rpmSLFirst )
                            m_brush->SetColor( float4(1,1,1,1) );
                        else if( lightRpm < ir_session.rpmSLLast )
                            m_brush->SetColor( warnCol );
                        else
                            m_brush->SetColor( float4(1,0,0,1) );
                        m_renderTarget->FillEllipse( &e, m_brush.Get() );
                    }
                }
            }

            // Gear & Speed
            {
                if (ir_RPM.getFloat() >= ir_session.rpmSLShift)
                {
                    m_brush->SetColor(shiftCol);
                    D2D1_RECT_F r = { m_boxGear.x0, m_boxGear.y0, m_boxGear.x1, m_boxGear.y1 };
                    m_renderTarget->FillRectangle(&r, m_brush.Get());
                }
                else if (ir_BrakeABSactive.getBool())
                {
                    m_brush->SetColor(badCol);
                    D2D1_RECT_F r = { m_boxGear.x0, m_boxGear.y0, m_boxGear.x1, m_boxGear.y1 };
                    m_renderTarget->FillRectangle(&r, m_brush.Get());
                }
                else if ( ir_EngineWarnings.getInt() & irsdk_revLimiterActive )
                {
                    m_brush->SetColor(warnCol);
                    D2D1_RECT_F r = { m_boxGear.x0, m_boxGear.y0, m_boxGear.x1, m_boxGear.y1 };
                    m_renderTarget->FillRectangle(&r, m_brush.Get());
                }
                else if ( ir_EngineWarnings.getInt() & irsdk_pitSpeedLimiter )
                {
                    m_brush->SetColor(pitCol);
                    D2D1_RECT_F r = { m_boxGear.x0, m_boxGear.y0, m_boxGear.x1, m_boxGear.y1 };
                    m_renderTarget->FillRectangle(&r, m_brush.Get());
                }
                m_brush->SetColor( textCol );

                const int gear = ir_Gear.getInt();
                char gearC = ' ';
                if( gear == -1 )
                    gearC = 'R';
                else if( gear == 0 )
                    gearC = 'N';
                else
                    gearC = char(gear + 48);
                swprintf( s, _countof(s), L"%C", gearC );
                m_text.render( m_renderTarget.Get(), s, m_textFormatGear.Get(), m_boxGear.x0, m_boxGear.x1, m_boxGear.y0+m_boxGear.h*0.41f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER );

                const float speedMps = ir_Speed.getFloat();
                if( speedMps >= 0 )
                {
                    float speed = 0;
                    if (speedUnit != "imperial")
                        speed = speedMps * 3.6f;
                    else
                        speed = speedMps * 2.23694f;
                    swprintf( s, _countof(s), L"%d", (int)(speed+0.5f) );
                    m_text.render( m_renderTarget.Get(), s, m_textFormatBold.Get(), m_boxGear.x0, m_boxGear.x1, m_boxGear.y0+m_boxGear.h*0.8f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER );
                }
            }
            
            // Laps
            {
                char lapsStr[32];
                
                const int totalLaps = ir_SessionLapsTotal.getInt();
            
                if( totalLaps == SHRT_MAX )
                    sprintf( lapsStr, "--" );
                else
                    sprintf( lapsStr, "%d", totalLaps );
                swprintf( s, _countof(s), L"%d / %S", currentLap, lapsStr );
                m_text.render( m_renderTarget.Get(), s, m_textFormat.Get(), m_boxLaps.x0, m_boxLaps.x1, m_boxLaps.y0+m_boxLaps.h*0.25f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER );

                if( remainingLaps < 0 )
                    sprintf( lapsStr, "--" );
                else if( sessionIsTimeLimited )
                    sprintf( lapsStr, "~%d", remainingLaps );
                else
                    sprintf( lapsStr, "%d", remainingLaps );
                swprintf( s, _countof(s), L"%S", lapsStr );
                m_text.render( m_renderTarget.Get(), s, m_textFormatLarge.Get(), m_boxLaps.x0, m_boxLaps.x1, m_boxLaps.y0+m_boxLaps.h*0.55f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER );

                m_text.render( m_renderTarget.Get(), L"TO GO", m_textFormatVerySmall.Get(), m_boxLaps.x0, m_boxLaps.x1, m_boxLaps.y0+m_boxLaps.h*0.75f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER );
            }

            // Position
            {
                const int pos = ir_getPosition( ir_session.driverCarIdx );
                if( pos )
                {
                    swprintf( s, _countof(s), L"%d", pos );
                    m_text.render( m_renderTarget.Get(), s, m_textFormatLarge.Get(), m_boxPos.x0, m_boxPos.x1, m_boxPos.y0+m_boxPos.h*0.5f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER );
                }
            }

            // Lap Delta
            {
                const int lapDelta = ir_getLapDeltaToLeader( ir_session.driverCarIdx, p1carIdx );
                if( lapDelta )
                {
                    swprintf( s, _countof(s), L"%d", lapDelta );
                    m_text.render( m_renderTarget.Get(), s, m_textFormatLarge.Get(), m_boxLapDelta.x0, m_boxLapDelta.x1, m_boxLapDelta.y0+m_boxLapDelta.h*0.5f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER );
                }
            }

            // Best time
            {
                // Figure out if we have the fastest lap across all cars
                bool haveFastestLap = false;
                {
                    int fastestLapCarIdx = -1;
                    float fastest = FLT_MAX;
                    for( int i=0; i<IR_MAX_CARS; ++i )
                    {
                        const Car& car = ir_session.cars[i];
                        if( car.isPaceCar || car.isSpectator || car.userName.empty() )
                            continue;

                        const float best = ir_CarIdxBestLapTime.getFloat(i);
                        if( best > 0 && best < fastest ) {
                            fastest = best;
                            fastestLapCarIdx = i;
                        }
                    }
                    haveFastestLap = fastestLapCarIdx == ir_session.driverCarIdx;
                }

                const float t = ir_LapBestLapTime.getFloat();
                if( t > 0 )
                {
                    bool vsb = true;
                    if( t < m_prevBestLapTime && tickCount-m_lastLapChangeTickCount < 5000 )  // blink
                        vsb = (tickCount % 800) < 500;
                    else
                        m_prevBestLapTime = t;

                    if( vsb )
                    {
                        D2D1_RECT_F r = { m_boxBest.x0, m_boxBest.y0, m_boxBest.x1, m_boxBest.y1 };
                        m_brush->SetColor( haveFastestLap ? fastestCol : goodCol );
                        m_renderTarget->FillRectangle( &r, m_brush.Get() );
                    }

                    m_brush->SetColor( textCol );
                    std::string str = formatLaptime( t );
                    m_text.render( m_renderTarget.Get(), toWide(str).c_str(), m_textFormat.Get(), m_boxBest.x0, m_boxBest.x1, m_boxBest.y0+m_boxBest.h*0.5f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER );
                }
            }

            // Last time
            {
                const float t = ir_LapLastLapTime.getFloat();
                if( t > 0 )
                {
                    std::string str = formatLaptime( t );
                    m_text.render( m_renderTarget.Get(), toWide(str).c_str(), m_textFormat.Get(), m_boxLast.x0, m_boxLast.x1, m_boxLast.y0+m_boxLast.h*0.5f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER );
                }
            }

            // P1's Last time
            {                
                if( p1carIdx >= 0 )
                {
                    const float t = ir_CarIdxLastLapTime.getFloat( p1carIdx );
                    if( t > 0 )
                    {
                        std::string str = formatLaptime( t );
                        m_text.render( m_renderTarget.Get(), toWide(str).c_str(), m_textFormat.Get(), m_boxP1Last.x0, m_boxP1Last.x1, m_boxP1Last.y0+m_boxP1Last.h*0.5f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER );
                    }
                }
            }

            // Fuel
            {
                const float xoff = 7;

                // Progress bar
                {
                    const float x0 = m_boxFuel.x0+xoff;
                    const float x1 = m_boxFuel.x1-xoff;
                    D2D1_RECT_F r = { x0, m_boxFuel.y0+12, x1, m_boxFuel.y0+m_boxFuel.h*0.11f };
                    m_brush->SetColor( float4( 0.5f, 0.5f, 0.5f, 0.5f ) );
                    m_renderTarget->FillRectangle( &r, m_brush.Get() );

                    const float fuelPct = ir_FuelLevelPct.getFloat();
                    r = { x0, m_boxFuel.y0+12, x0+fuelPct*(x1-x0), m_boxFuel.y0+m_boxFuel.h*0.11f };
                    m_brush->SetColor( fuelPct < 0.1f ? warnCol : goodCol );
                    m_renderTarget->FillRectangle( &r, m_brush.Get() );
                }
                
                m_brush->SetColor( textCol );
                m_text.render( m_renderTarget.Get(), L"Laps", m_textFormat.Get(),      m_boxFuel.x0+xoff, m_boxFuel.x1, m_boxFuel.y0+m_boxFuel.h*2.3f/12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_LEADING );
                m_text.render( m_renderTarget.Get(), L"Rem", m_textFormatSmall.Get(), m_boxFuel.x0+xoff, m_boxFuel.x1, m_boxFuel.y0+m_boxFuel.h*4.6f/12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_LEADING );
                m_text.render( m_renderTarget.Get(), L"Per", m_textFormatSmall.Get(), m_boxFuel.x0+xoff, m_boxFuel.x1, m_boxFuel.y0+m_boxFuel.h*6.4f/12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_LEADING );
                m_text.render(m_renderTarget.Get(), L"Fin+", m_textFormatSmall.Get(), m_boxFuel.x0 + xoff, m_boxFuel.x1, m_boxFuel.y0 + m_boxFuel.h * 8.2f / 12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_LEADING);
                if (targetLap == 0) {
                    m_text.render(m_renderTarget.Get(), L"Add", m_textFormatSmall.Get(), m_boxFuel.x0 + xoff, m_boxFuel.x1, m_boxFuel.y0 + m_boxFuel.h * 10.0f / 12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_LEADING);
                }
                else {
                    swprintf(s, _countof(s), L"TgtFuel-%d", targetLap);
                    m_text.render(m_renderTarget.Get(), s, m_textFormatSmall.Get(), m_boxFuel.x0 + xoff, m_boxFuel.x1, m_boxFuel.y0 + m_boxFuel.h * 10.0f / 12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_LEADING);
                }
                
                const float estimateFactor = g_cfg.getFloat( m_name, "fuel_estimate_factor", 1.1f );
                const float fuelReserveMargin = g_cfg.getFloat(m_name, "fuel_reserve_margin", 0.25f);
                const float remainingFuel  = ir_FuelLevel.getFloat();

                // Update average fuel consumption tracking. Ignore laps that weren't entirely under green or where we pitted.
                float avgPerLap = 0;
                {
                    if( lapCountUpdated )
                    {
                        const float usedLastLap = std::max( 0.0f, m_lapStartRemainingFuel - remainingFuel );
                        m_lapStartRemainingFuel = remainingFuel;
                        
                        // When resetting, the lap count resets and pushes two 0.0L laps, so we skip them here
                        if (m_isValidFuelLap && usedLastLap > 0.0f) {
                            m_fuelUsedLastLaps.push_back( usedLastLap );
#ifdef _DEBUG
                            printf("Pushing fuel lap: %f\n", usedLastLap);
#endif
                        }

                        const int numLapsToAvg = g_cfg.getInt( m_name, "fuel_estimate_avg_green_laps", 4 );
                        while( m_fuelUsedLastLaps.size() > numLapsToAvg )
                            m_fuelUsedLastLaps.pop_front();

                        m_isValidFuelLap = true;
                    }
                    
                    // For Test Drive or solo practice
                    const int flagStatus = (ir_SessionFlags.getInt() & ((((int)ir_session.sessionType != 0) ? irsdk_oneLapToGreen : 0) | irsdk_yellow | irsdk_yellowWaving | irsdk_red | irsdk_checkered | irsdk_crossed | irsdk_caution | irsdk_cautionWaving | irsdk_disqualify | irsdk_repair));
                    if (flagStatus != 0 || ir_CarIdxOnPitRoad.getBool(carIdx)) {
                        dbg("flagStatus: 0x%X", flagStatus);
                        m_isValidFuelLap = false;
                    }
                    
                    
                    for( float v : m_fuelUsedLastLaps ) {
                        avgPerLap += v;
                        dbg("%f",v);
                    }
                    if( !m_fuelUsedLastLaps.empty() )
                        avgPerLap /= (float)m_fuelUsedLastLaps.size();

                    dbg( "valid fuel lap: %d", (int)m_isValidFuelLap );
                }

                // Est Laps
                const float perLapConsEst = avgPerLap * estimateFactor;  // conservative estimate of per-lap use for further calculations
                if( perLapConsEst > 0 )
                {
                    const float estLaps = (remainingFuel-fuelReserveMargin) / perLapConsEst;
                    swprintf( s, _countof(s), L"%.*f", g_cfg.getInt( m_name, "fuel_decimal_places", 2), estLaps);
                    m_text.render( m_renderTarget.Get(), s, m_textFormatBold.Get(), m_boxFuel.x0, m_boxFuel.x1-xoff, m_boxFuel.y0+m_boxFuel.h*3.0f/12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_TRAILING );
                }

                // Remaining
                if( remainingFuel >= 0 )
                {
                    float val = remainingFuel;
                    if( imperial )
                        val *= 0.264172f;
                    swprintf( s, _countof(s), imperial ? L"%.2f gl" : L"%.2f lt", val );
                    m_text.render( m_renderTarget.Get(), s, m_textFormat.Get(), m_boxFuel.x0, m_boxFuel.x1-xoff, m_boxFuel.y0+m_boxFuel.h*5.3f/12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_TRAILING );
                }

                // Per Lap
                if( avgPerLap > 0 )
                {
                    float val = avgPerLap;
                    if( imperial )
                        val *= 0.264172f;
                    swprintf( s, _countof(s), imperial ? L"%.2f gl" : L"%.2f lt", val );
                    m_text.render( m_renderTarget.Get(), s, m_textFormat.Get(), m_boxFuel.x0, m_boxFuel.x1-xoff, m_boxFuel.y0+m_boxFuel.h*7.1f/12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_TRAILING );
                }
                else {
                    swprintf(s, _countof(s), L"%.2f ERR", avgPerLap);
                    m_text.render(m_renderTarget.Get(), s, m_textFormat.Get(), m_boxFuel.x0, m_boxFuel.x1 - xoff, m_boxFuel.y0 + m_boxFuel.h * 7.1f / 12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_TRAILING);
                }

                // To Finish
                if( remainingLaps >= 0 && perLapConsEst > 0 )
                {
                    
                    float toFinish;

                    if (targetLap == 0) {
                        toFinish = std::max(0.0f, remainingLaps * perLapConsEst - (remainingFuel - fuelReserveMargin));
                    } else {
                        toFinish = (targetLap+1-currentLap) * perLapConsEst - (m_lapStartRemainingFuel - fuelReserveMargin);
                    }

                    if( toFinish > ir_PitSvFuel.getFloat() || (toFinish>0 && !ir_dpFuelFill.getFloat())  )
                        m_brush->SetColor( warnCol );
                    else 
                        m_brush->SetColor( goodCol );

                    if( imperial )
                        toFinish *= 0.264172f;
                    swprintf( s, _countof(s), imperial ? L"%3.2f gl" : L"%3.2f lt", toFinish );
                    m_text.render( m_renderTarget.Get(), s, m_textFormat.Get(), m_boxFuel.x0, m_boxFuel.x1-xoff, m_boxFuel.y0+m_boxFuel.h*8.9f/12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_TRAILING );
                    m_brush->SetColor( textCol );
                }

                // Add
                float add = ir_PitSvFuel.getFloat();
                if (targetLap != 0) {

                    float targetFuel = (m_lapStartRemainingFuel - fuelReserveMargin) / ( targetLap + 1 - currentLap);

                    if (imperial)
                        targetFuel *= 0.264172f;
                    swprintf(s, _countof(s), imperial ? L"%3.2f gl" : L"%3.2f lt", targetFuel);
                    m_text.render(m_renderTarget.Get(), s, m_textFormat.Get(), m_boxFuel.x0, m_boxFuel.x1 - xoff, m_boxFuel.y0 + m_boxFuel.h * 10.7f / 12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_TRAILING);
                    m_brush->SetColor(textCol);
                }
                else if( add >= 0 )
                {
                    if (ir_dpFuelFill.getFloat())
                        m_brush->SetColor(serviceCol);

                    if( imperial )
                        add *= 0.264172f;
                    swprintf( s, _countof(s), imperial ? L"%3.2f gl" : L"%3.2f lt", add );
                    m_text.render( m_renderTarget.Get(), s, m_textFormat.Get(), m_boxFuel.x0, m_boxFuel.x1-xoff, m_boxFuel.y0+m_boxFuel.h*10.7f/12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_TRAILING );
                    m_brush->SetColor( textCol );
                }
            }

            // Tires
            {
                if (g_cfg.getBool(m_name, "show_tire_wear", false)) {
                    const float lf = 100.0f * std::min(std::min(ir_LFwearL.getFloat(), ir_LFwearM.getFloat()), ir_LFwearR.getFloat());
                    const float rf = 100.0f * std::min(std::min(ir_RFwearL.getFloat(), ir_RFwearM.getFloat()), ir_RFwearR.getFloat());
                    const float lr = 100.0f * std::min(std::min(ir_LRwearL.getFloat(), ir_LRwearM.getFloat()), ir_LRwearR.getFloat());
                    const float rr = 100.0f * std::min(std::min(ir_RRwearL.getFloat(), ir_RRwearM.getFloat()), ir_RRwearR.getFloat());

                    int tireChangeMask = 0;

                    // Open wheelers, cars with ONE Replace box
                    if (ir_dpTireChange.isValid()) {
                        tireChangeMask = ir_dpTireChange.getInt() * 0xF;
                    }
                    // Oval cars, L/R boxes
                    else if (ir_dpLTireChange.isValid()) {
                        tireChangeMask =
                            ir_dpLTireChange.getInt() * (irsdk_LFTireChange + irsdk_LRTireChange)
                            +
                            ir_dpRTireChange.getInt() * (irsdk_RFTireChange + irsdk_RRTireChange);
                    }

                    // Any other, if we can change individuals, we can change all
                    else if (ir_dpLFTireChange.isValid()) {
                        tireChangeMask =
                            ir_dpLFTireChange.getInt() * irsdk_LFTireChange
                            + ir_dpLRTireChange.getInt() * irsdk_LRTireChange
                            + ir_dpRFTireChange.getInt() * irsdk_RFTireChange
                            + ir_dpRRTireChange.getInt() * irsdk_RRTireChange;
                    }

                    // Left
                    if (tireChangeMask & irsdk_LFTireChange)
                        m_brush->SetColor(serviceCol);
                    else
                        m_brush->SetColor(textCol);
                    swprintf(s, _countof(s), L"%d%%", (int)(lf + 0.5f));
                    m_text.render(m_renderTarget.Get(), s, m_textFormatSmall.Get(), m_boxTires.x0 + 6, m_boxTires.x0 + m_boxTires.w / 2, m_boxTires.y0 + m_boxTires.h * 1.0f / 3.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
                    if (tireChangeMask & irsdk_LRTireChange)
                        m_brush->SetColor(serviceCol);
                    else
                        m_brush->SetColor(textCol);
                    swprintf(s, _countof(s), L"%d%%", (int)(lr + 0.5f));
                    m_text.render(m_renderTarget.Get(), s, m_textFormatSmall.Get(), m_boxTires.x0 + 6, m_boxTires.x0 + m_boxTires.w / 2, m_boxTires.y0 + m_boxTires.h * 2.0f / 3.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);

                    // Right
                    if (tireChangeMask & irsdk_RFTireChange)
                        m_brush->SetColor(serviceCol);
                    else
                        m_brush->SetColor(textCol);
                    swprintf(s, _countof(s), L"%d%%", (int)(rf + 0.5f));
                    m_text.render(m_renderTarget.Get(), s, m_textFormatSmall.Get(), m_boxTires.x0 + m_boxTires.w / 2, m_boxTires.x1 - 6, m_boxTires.y0 + m_boxTires.h * 1.0f / 3.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
                    if (tireChangeMask & irsdk_RRTireChange)
                        m_brush->SetColor(serviceCol);
                    else
                        m_brush->SetColor(textCol);
                    swprintf(s, _countof(s), L"%d%%", (int)(rr + 0.5f));
                    m_text.render(m_renderTarget.Get(), s, m_textFormatSmall.Get(), m_boxTires.x0 + m_boxTires.w / 2, m_boxTires.x1 - 6, m_boxTires.y0 + m_boxTires.h * 2.0f / 3.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
                    m_brush->SetColor(textCol);
				}
                else {
                    float lf = (ir_LFtempCL.getFloat() + ir_LFtempCM.getFloat() + ir_LFtempCR.getFloat()) / 3;
					float rf = (ir_RFtempCL.getFloat() + ir_RFtempCM.getFloat() + ir_RFtempCR.getFloat()) / 3;
					float lr = (ir_LRtempCL.getFloat() + ir_LRtempCM.getFloat() + ir_LRtempCR.getFloat()) / 3;
					float rr = (ir_RRtempCL.getFloat() + ir_RRtempCM.getFloat() + ir_RRtempCR.getFloat()) / 3;

                    if ( imperial ) {
                        lf = celsiusToFahrenheit(lf);
                        rf = celsiusToFahrenheit(rf);
                        lr = celsiusToFahrenheit(lr);
                        rr = celsiusToFahrenheit(rr);
                    }

                    // Left
                    swprintf(s, _countof(s), L"%0.1f�", lf);
                    m_text.render(m_renderTarget.Get(), s, m_textFormatSmall.Get(), m_boxTires.x0 + 6, m_boxTires.x0 + m_boxTires.w / 2, m_boxTires.y0 + m_boxTires.h * 1.0f / 3.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
                    
                    swprintf(s, _countof(s), L"%0.1f�", lr);
                    m_text.render(m_renderTarget.Get(), s, m_textFormatSmall.Get(), m_boxTires.x0 + 6, m_boxTires.x0 + m_boxTires.w / 2, m_boxTires.y0 + m_boxTires.h * 2.1f / 3.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);

                    // Right
                    swprintf(s, _countof(s), L"%0.1f�", rf);
                    m_text.render(m_renderTarget.Get(), s, m_textFormatSmall.Get(), m_boxTires.x0 + m_boxTires.w / 2, m_boxTires.x1 - 6, m_boxTires.y0 + m_boxTires.h * 1.0f / 3.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
                    
                    swprintf(s, _countof(s), L"%0.1f�", rr);
                    m_text.render(m_renderTarget.Get(), s, m_textFormatSmall.Get(), m_boxTires.x0 + m_boxTires.w / 2, m_boxTires.x1 - 6, m_boxTires.y0 + m_boxTires.h * 2.1f / 3.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
                    m_brush->SetColor(textCol);
                }
            }

            // Delta
            {
                if( ir_LapDeltaToSessionBestLap_OK.getBool() )
                {
                    const float t = ir_LapDeltaToSessionBestLap.getFloat();
                    swprintf( s, _countof(s), L"%+4.2f", t );

                    D2D1_RECT_F r = { m_boxDelta.x0, m_boxDelta.y0, m_boxDelta.x1, m_boxDelta.y1 };
                    m_brush->SetColor( t <= 0 ? goodCol : badCol );
                    m_renderTarget->FillRectangle( &r, m_brush.Get() );
                    m_brush->SetColor( textCol );
                    
                    // Don't cache this! The memory cost is too high for a number that could skyrocket if you stop on track.
                    // Weird edge case, but the CPU cost is negligible vs the risk of this crashing a computer
                    m_text.render( m_renderTarget.Get(), s, m_textFormat.Get(), m_boxDelta.x0, m_boxDelta.x1, m_boxDelta.y0+m_boxDelta.h*0.5f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER, true);
                }
            }

            // Session
            {                   
                const double sessionTime = remainingSessionTime>=0 ? remainingSessionTime : ir_SessionTime.getDouble();

                const int    hours = int( sessionTime / 3600.0 );
                const int    mins  = int( sessionTime / 60.0 ) % 60;
                const int    secs  = (int)fmod( sessionTime, 60.0 );
                if( hours )
                    swprintf( s, _countof(s), L"%d:%02d:%02d", hours, mins, secs );
                else
                    swprintf( s, _countof(s), L"%02d:%02d", mins, secs ); 
                m_text.render( m_renderTarget.Get(), s, m_textFormatSmall.Get(), m_boxSession.x0, m_boxSession.x1, m_boxSession.y0+m_boxSession.h*0.55f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER );
            }

            // Incidents
            {
                const int inc = ir_PlayerCarTeamIncidentCount.getInt();
                swprintf( s, _countof(s), L"%dx", inc );
                m_text.render( m_renderTarget.Get(), s, m_textFormat.Get(), m_boxInc.x0, m_boxInc.x1, m_boxInc.y0+m_boxInc.h*0.5f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER );
            }

            // Brake bias
            {
                const float bias = ir_dcBrakeBias.getFloat();
                if (m_prevBrakeBias == 0) m_prevBrakeBias = bias;
                if (m_prevBrakeBias != bias) m_prevBrakeBiasTickCount = tickCount;
                if (m_prevBrakeBiasTickCount+500 > tickCount)
                {
                    m_brush->SetColor(warnCol);
                    D2D1_RECT_F r = { m_boxBias.x0, m_boxBias.y0, m_boxBias.x1, m_boxBias.y1 };
                    m_renderTarget->FillRectangle(&r, m_brush.Get());
                }
                m_brush->SetColor(textCol);
                m_prevBrakeBias = bias;
                swprintf( s, _countof(s), L"%+3.1f", bias );
                m_text.render( m_renderTarget.Get(), s, m_textFormat.Get(), m_boxBias.x0, m_boxBias.x1, m_boxBias.y0+m_boxBias.h*0.5f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER );
            }

            // Oil temp
            {
                // m_brush->SetColor(textCol);
                float temp = ir_OilTemp.getFloat();
                if( imperial )
                    temp = celsiusToFahrenheit( temp );

                if( ir_EngineWarnings.getInt() & irsdk_oilTempWarning )
					if ( m_OilWarnTickCount+1000 > tickCount )
                    {
                        m_brush->SetColor( warnCol );
                        D2D1_RECT_F r = { m_boxOil.x0, m_boxOil.y0, m_boxOil.x1, m_boxOil.y1 };
                        m_renderTarget->FillRectangle(&r, m_brush.Get());
                        m_brush->SetColor( textCol );
					}
					else
                        m_brush->SetColor( warnCol );
                        if ( m_OilWarnTickCount+1500 < tickCount )
    						m_OilWarnTickCount = tickCount;

                swprintf( s, _countof(s), L"%3.0f�", temp );
                m_text.render( m_renderTarget.Get(), s, m_textFormat.Get(), m_boxOil.x0, m_boxOil.x1, m_boxOil.y0+m_boxOil.h*0.5f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER );
                m_brush->SetColor( textCol );
            }

            // Water temp
            {
                float temp = ir_WaterTemp.getFloat();
                if( imperial )
                    temp = celsiusToFahrenheit( temp );

                if( ir_EngineWarnings.getInt() & irsdk_waterTempWarning )
                    if ( m_WaterWarnTickCount+1000 > tickCount )
                    {
                        m_brush->SetColor( warnCol );
                        D2D1_RECT_F r = { m_boxWater.x0, m_boxWater.y0, m_boxWater.x1, m_boxWater.y1 };
                        m_renderTarget->FillRectangle(&r, m_brush.Get());
						m_brush->SetColor(textCol);
					}
                    else
                        m_brush->SetColor( warnCol );
				        if (m_WaterWarnTickCount + 1500 < tickCount)
					        m_WaterWarnTickCount = tickCount;

                swprintf( s, _countof(s), L"%3.0f�", temp );
                m_text.render( m_renderTarget.Get(), s, m_textFormat.Get(), m_boxWater.x0, m_boxWater.x1, m_boxWater.y0+m_boxWater.h*0.5f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER );
                m_brush->SetColor( textCol );
            }

            // ABS
            {
                const int abs = ir_dcABS.getInt();
                if (m_prevABS == 0) m_prevABS = abs;
                if (m_prevABS != abs) m_prevABSTickCount = tickCount;
                if (m_prevABSTickCount + 500 > tickCount)
                {
                    m_brush->SetColor(warnCol);
                    D2D1_RECT_F r = { m_boxABS.x0, m_boxABS.y0, m_boxABS.x1, m_boxABS.y1 };
                    m_renderTarget->FillRectangle(&r, m_brush.Get());
                }
                m_brush->SetColor(textCol);
                m_prevABS = abs;
                swprintf(s, _countof(s), L"%d", abs);
                m_text.render(m_renderTarget.Get(), s, m_textFormat.Get(), m_boxABS.x0, m_boxABS.x1, m_boxABS.y0 + m_boxABS.h * 0.5f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
                m_brush->SetColor(textCol);
            }

			// TC
			{
				const int tc = ir_dcTractionControl.getInt();
				if (m_prevTC == 0) m_prevTC = tc;
				if (m_prevTC != tc) m_prevTCTickCount = tickCount;
				if (m_prevTCTickCount + 500 > tickCount)
				{
					m_brush->SetColor(warnCol);
					D2D1_RECT_F r = { m_boxTC.x0, m_boxTC.y0, m_boxTC.x1, m_boxTC.y1 };
					m_renderTarget->FillRectangle(&r, m_brush.Get());
				}
				m_brush->SetColor(textCol);
				m_prevTC = tc;
				swprintf(s, _countof(s), L"%d", tc);
				m_text.render(m_renderTarget.Get(), s, m_textFormat.Get(), m_boxTC.x0, m_boxTC.x1, m_boxTC.y0 + m_boxTC.h * 0.5f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
				m_brush->SetColor(textCol);
			}


            m_renderTarget->EndDraw();
        }

        void addBoxFigure( ID2D1GeometrySink* geometrySink, const Box& box )
        {
            if( !box.title.empty() )
            {
                const float hctr = (box.x0 + box.x1) * 0.5f;
                const float titleWidth = std::min( box.w, 6 + m_text.getExtent( toWide(box.title).c_str(), m_textFormat.Get(), box.x0, box.x1, DWRITE_TEXT_ALIGNMENT_CENTER ).x );
                geometrySink->BeginFigure( float2(hctr-titleWidth/2,box.y0), D2D1_FIGURE_BEGIN_HOLLOW );
                geometrySink->AddLine( float2(box.x0,box.y0) );
                geometrySink->AddLine( float2(box.x0,box.y1) );
                geometrySink->AddLine( float2(box.x1,box.y1) );
                geometrySink->AddLine( float2(box.x1,box.y0) );
                geometrySink->AddLine( float2(hctr+titleWidth/2,box.y0) );
                geometrySink->EndFigure( D2D1_FIGURE_END_OPEN );
            }
            else
            {
                geometrySink->BeginFigure( float2(box.x0,box.y0), D2D1_FIGURE_BEGIN_HOLLOW );
                geometrySink->AddLine( float2(box.x0,box.y1) );
                geometrySink->AddLine( float2(box.x1,box.y1) );
                geometrySink->AddLine( float2(box.x1,box.y0) );
                geometrySink->EndFigure( D2D1_FIGURE_END_CLOSED );
            }
        }

        float r2ax( float rx )
        {
            return rx * (float)m_width;
        }

        float r2ay( float ry )
        {
            return ry * (float)m_height;
        }

        Box makeBox( float x0, float w, float y0, float h, const std::string& title )
        {
            Box r;

            if( w <= 0 || h <= 0 )
                return r;

            r.x0 = r2ax( x0 );
            r.x1 = r2ax( x0+w );
            r.y0 = r2ay( y0 );
            r.y1 = r2ay( y0+h );
            r.w = r.x1 - r.x0;
            r.h = r.y1 - r.y0;
            r.title = title;
            return r;
        }

    protected:

        virtual bool hasCustomBackground()
        {
            return true;
        }

        Box m_boxGear;
        Box m_boxLaps;
        Box m_boxPos;
        Box m_boxLapDelta;
        Box m_boxBest;
        Box m_boxLast;
        Box m_boxP1Last;
        Box m_boxDelta;
        Box m_boxSession;
        Box m_boxInc;
        Box m_boxBias;
        Box m_boxFuel;
        Box m_boxTires;
        Box m_boxOil;
        Box m_boxWater;
        Box m_boxABS;
		Box m_boxTC;

        Microsoft::WRL::ComPtr<IDWriteTextFormat>  m_textFormat;
        Microsoft::WRL::ComPtr<IDWriteTextFormat>  m_textFormatBold;
        Microsoft::WRL::ComPtr<IDWriteTextFormat>  m_textFormatLarge;
        Microsoft::WRL::ComPtr<IDWriteTextFormat>  m_textFormatSmall;
        Microsoft::WRL::ComPtr<IDWriteTextFormat>  m_textFormatVerySmall;
        Microsoft::WRL::ComPtr<IDWriteTextFormat>  m_textFormatGear;

        Microsoft::WRL::ComPtr<ID2D1PathGeometry1> m_boxPathGeometry;
        Microsoft::WRL::ComPtr<ID2D1PathGeometry1> m_backgroundPathGeometry;

        TextCache           m_text;
        Microsoft::WRL::ComPtr<ID2D1Bitmap> m_backgroundBitmap;

        int                 m_prevCurrentLap = 0;
        DWORD               m_lastLapChangeTickCount = 0;

        float               m_prevBestLapTime = 0;
        
        float               m_prevBrakeBias = 0;
        DWORD               m_prevBrakeBiasTickCount = 0;

        float               m_prevABS = 0;
        DWORD               m_prevABSTickCount = 0;

		float               m_prevTC = 0;
		DWORD               m_prevTCTickCount = 0;

        DWORD               m_OilWarnTickCount = 0;

        DWORD               m_WaterWarnTickCount = 0;

        float               m_lapStartRemainingFuel = 0;
        std::deque<float>   m_fuelUsedLastLaps;
        bool                m_isValidFuelLap = false;
};

