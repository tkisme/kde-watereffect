/********************************************************************
Copyright (C) 2008 Michal Srb <michalsrb@gmail.com>
Inspired by code of other kwin effects.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/

#ifndef KWIN_DEMO_WATEREFFEKT_H
#define KWIN_DEMO_WATEREFFEKT_H

#include <kwineffects.h>
#include <vector>

namespace KWin
{

class GLRenderTarget;
class GLTexture;

class WaterEffect : public QObject, public Effect
{
  Q_OBJECT
  class Wave
  {
    public:
      Wave() {};
      virtual ~Wave() {};
      virtual float getHeight(float x, float y, float time) { return 0.0f; };
      virtual bool isIn(float y, float time) { return false; };
      virtual bool isReadyToDelete(float time) { return true; };
  };

  class CircleWave : public Wave
  {
    public:
      float x, y; // Position in px
      float amplitude; // Amplitude in ??
      float periodeLength; // Length of wave [px] / 2pi
      float damping, radius; // Radius in px of start and end of blending.
      float periods; // Number of periods. * periodeLength / 2
      float birthTime; // 
      float speed; // Pixel per milisec
      int kvadrant;

      CircleWave(const float c_x, const float c_y, const float c_amplitude, const float c_periodeLength, const float c_damping, const float c_radius, const float c_periods, const float c_birthTime, const float c_speed, const int c_kvadrant = 0) : x(c_x), y(c_y), amplitude(c_amplitude), periodeLength(c_periodeLength), damping(c_damping), radius(c_radius), periods(c_periods), birthTime(c_birthTime), speed(c_speed), kvadrant(c_kvadrant) {};
      virtual ~CircleWave() {};
      virtual float getHeight(float x, float y, float time);
      virtual bool isIn(float y, float time);
      virtual bool isReadyToDelete(float time);
  };

  class LineWave : public CircleWave
  {
    public:
//       float x, y; // Position in px
//       float amplitude; // Amplitude in ??
//       float periodeLength; // Length of wave [px] / 2pi
//       float damping, radius; // Diameter in px of start and end of blending.
//       float periods; // Number of periods. * periodeLength / 2
//       float birthTime; // 
//       float speed; // Pixel per milisec
//       int kvadrant;
// 
      float size;

      LineWave(const float c_x, const float c_y, const float c_amplitude, const float c_periodeLength, const float c_damping, const float c_radius, const float c_periods, const float c_birthTime, const float c_speed, const int c_kvadrant, const float c_size) : CircleWave(c_x, c_y, c_amplitude, c_periodeLength, c_damping, c_radius, c_periods, c_birthTime, c_speed, c_kvadrant), size(c_size) {};
      virtual ~LineWave() {};
      virtual float getHeight(float x, float y, float time);
      virtual bool isIn(float y, float time);
      virtual bool isReadyToDelete(float time);
  };

  public:
    WaterEffect();
    ~WaterEffect();
    virtual void reconfigure(/*ReconfigureFlags*/);
    virtual void prePaintScreen(ScreenPrePaintData& data, int time);
    virtual void paintScreen( int mask, QRegion region, ScreenPaintData& data );
    virtual void mouseChanged(const QPoint &pos, const QPoint &oldpos, Qt::MouseButtons buttons, Qt::MouseButtons oldbuttons, Qt::KeyboardModifiers modifiers, Qt::KeyboardModifiers oldmodifiers);
//     virtual void windowUnminimized(EffectWindow* w);
    virtual void windowActivated(EffectWindow* w);

    void roundedRectangle(float left, float top, float right, float bottom);

    static bool supported();


  public slots:
    void toggleRain();
    void eraseWaves();

  protected:
    void loadConfig(QString config);

  private:
    float getHeight(float x, float y);
    bool isIn(float y);
    void deleteDeaths();
    bool isSomethingToPaint() { return waves.size() > 0; };
    void letRain();

    GLTexture* texture;
    GLRenderTarget* renderTarget;
    bool valid;

    bool NPOTsupported;
    float gridSize;
    float xSize;
    float ySize;
    float xCount;
    float yCount;

    float* lastlineHeights;
    float* lastlineXMoves;
    float* lastlineYMoves;
    float* lastHeights;
    float* lastXMoves;
    float* lastYMoves;

    std::vector<Wave*> waves;
    float time;

    bool rain;
    float rainLastDrop;

    // settings:
    float rainDelay;
    bool mouse;
    bool activateWave;

    float defaultAmplitude;
    float defaultPeriodeLength;
    float defaultDamping, defaultRadius;
    float defaultSpeed;
};

} // namespace

#endif
