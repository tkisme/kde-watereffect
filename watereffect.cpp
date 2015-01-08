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

#include "watereffect.h"

#include <kwinglutils.h>
#include <kstandarddirs.h>
#include <kactioncollection.h>
#include <kaction.h>
#include <klocale.h>
#include <kconfiggroup.h>
#include <kdebug.h>

#include <GL/gl.h>
#include <math.h>

namespace KWin
{

KWIN_EFFECT( watereffect, WaterEffect )
KWIN_EFFECT_SUPPORTED( watereffect, WaterEffect::supported() )

// WaterEffect class:

WaterEffect::WaterEffect()
    : texture(0)
    , renderTarget(0)
    , valid(true)
    , rain(false)
    , time(0.0f)

{
  KActionCollection* actionCollection = new KActionCollection( this );

  KAction* a = (KAction*)actionCollection->addAction("ToggleRain");
  a->setText(i18n("Toggle Rain"));
  a->setGlobalShortcut(KShortcut(/*Qt::SHIFT + Qt::META + Qt::Key_R*/));
  connect(a, SIGNAL(triggered(bool)), this, SLOT(toggleRain()));

  a = (KAction*)actionCollection->addAction("EraseWaves");
  a->setText(i18n("Erase Waves"));
  a->setGlobalShortcut(KShortcut(/* Qt::CTRL + Qt::META + Qt::Key_E */));
  connect(a, SIGNAL(triggered(bool)), this, SLOT(eraseWaves()));

  reconfigure(/*ReconfigureAll*/);

  lastlineHeights = new float[int(xCount) + 1];
  lastlineXMoves = new float[int(xCount) + 1];
  lastlineYMoves = new float[int(xCount) + 1];
  lastHeights = new float[int(xCount) + 1];
  lastXMoves = new float[int(xCount) + 1];
  lastYMoves = new float[int(xCount) + 1];
}

WaterEffect::~WaterEffect()
{
  delete texture;
  delete renderTarget;

  delete[] lastlineHeights; // With WATEREFFECT_USE_STRIP we possibly can get rid of some of these arrays.
  delete[] lastlineXMoves;
  delete[] lastlineYMoves;
  delete[] lastHeights;
  delete[] lastXMoves;
  delete[] lastYMoves;
}

void WaterEffect::reconfigure(/*ReconfigureFlags*/)
{
  KConfigGroup conf = effects->effectConfig("WaterEffect");
  gridSize = (float)conf.readEntry("GridSize", 10);
  int texw = displayWidth();
  int texh = displayHeight();
  xCount = texw / gridSize;
  yCount = texh / gridSize;
  xSize = 1.0f / xCount;
  ySize = 1.0f / yCount;
  NPOTsupported = GLTexture::NPOTTextureSupported();
  if(!NPOTsupported) {
    kWarning(1212) << "NPOT textures not supported, wasting some memory";
    texw = nearestPowerOfTwo(texw);
    texh = nearestPowerOfTwo(texh);
    xSize = 1.0f * gridSize / float(texw);
    ySize = 1.0f * gridSize / float(texh);
  }
  if(texture) delete texture;
  texture = new GLTexture(texw, texh);
  texture->setFilter(GL_NEAREST_MIPMAP_LINEAR); // I was searching which filter would be best. In my opinion, this effect looks best with the worse filter, so I hardcoded it without respect to global settings.
  if(renderTarget) delete renderTarget;
  renderTarget = new GLRenderTarget(*texture);
  if(!renderTarget->valid())
    valid = false;

  rainDelay = (float)conf.readEntry("RainDelay", 30);
  mouse = conf.readEntry("Mouse", true);
  activateWave = conf.readEntry("ActivateWave", false);

  defaultAmplitude = (float)conf.readEntry("defaultAmplitude", 40);
  defaultPeriodeLength = (float)conf.readEntry("defaultPeriodeLength", 80)/(2.0f*3.14f);
  defaultDamping = (float)conf.readEntry("defaultDamping", 100);
  defaultRadius = (float)conf.readEntry("defaultRadius", 250);
  defaultSpeed = (float)conf.readEntry("defaultSpeed", 250)/1000.0f;
}

bool WaterEffect::supported()
{
  return GLRenderTarget::supported() && effects->compositingType() == OpenGLCompositing;
}

void WaterEffect::toggleRain()
{
  rain = !rain;
}

void WaterEffect::eraseWaves()
{
  waves.clear();
}

void WaterEffect::prePaintScreen(ScreenPrePaintData& data, int elapsed_time)
{
  effects->prePaintScreen(data, elapsed_time);
  if(!valid) return;
  time += float(elapsed_time);
  deleteDeaths();
  if(rain) letRain();
}

float WaterEffect::getHeight(float x, float y)
{
  float toReturn = 0.0f;
  for(std::vector<Wave*>::iterator iter = waves.begin(); iter != waves.end(); iter++)
    toReturn += (*iter)->getHeight(x, y, time);
  return toReturn;
}

bool WaterEffect::isIn(float y)
{
  for(std::vector<Wave*>::iterator iter = waves.begin(); iter != waves.end(); iter++)
    if((*iter)->isIn(y, time)) return true;
  return false;
}

void WaterEffect::deleteDeaths()
{
  for(std::vector<Wave*>::iterator iter = waves.begin(); iter != waves.end(); iter++) {
    if((*iter)->isReadyToDelete(time)) {
      delete (*iter);
      waves.erase(iter);
      break;
    }
  }
}

void WaterEffect::paintScreen( int mask, QRegion region, ScreenPaintData& data )
{
  if(!valid || !isSomethingToPaint()) {
    effects->paintScreen( mask, region, data );
    return;
  }
  //effects->pushRenderTarget(renderTarget);
  effects->paintScreen( mask, region, data );
  //effects->popRenderTarget();

  texture->bind();

//   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);

  for(int i = 0; i < xCount + 1; i++) { // Zero what we need:
    lastlineHeights[i] = 0.0f;
    lastlineXMoves[i] = 0.0f;
    lastlineYMoves[i] = 0.0f;
  }
  lastHeights[0] = 0.0f;
  lastXMoves[0] = 0.0f;
  lastYMoves[0] = 0.0f;

  float skippedStart = -1.0;

  for(float y = 0.0f; y < yCount; y++) {
    if(!isIn(y*gridSize) && y + 1.0f < yCount) { // This line doesn't contain any wave and it isn't last line of the screen, so we can skip it..
      if(skippedStart == -1.0) skippedStart = y; // If it is first skipped line, save it..
      continue;                                  // Skip to next..
    }
    if(y + 1.0f > yCount) y++;                   // If this is last line of the screen, fill skipped area to the end. (Ok, tiny waves visible only in the last line will disappear, we reconcile with it.)
    if(skippedStart != -1.0) {                   // So, this line wont be skipped. Is there some area which was skipped above?
      glBegin(GL_QUADS);                         // Yes, it was, so paint it..
//         glColor3f(0.0f, 0.0f, 0.0f); // No color change..
        glTexCoord2f(0.0f, -ySize * skippedStart); // Left top
        glVertex2f(0.0f, gridSize * skippedStart);
        glTexCoord2f(0.0f, -ySize * y); // Left bottom
        glVertex2f(0.0f, gridSize * y);
        if(NPOTsupported) {
          glTexCoord2f(1.0, -ySize * y); // Right bottom
          glVertex2f(displayWidth(), gridSize * y);
          glTexCoord2f(1.0, -ySize * skippedStart); // Right top
        } else {
          glTexCoord2f(xSize * xCount, -ySize * y); // Right bottom
          glVertex2f(displayWidth(), gridSize * y);
          glTexCoord2f(xSize * xCount, -ySize * skippedStart); // Right top
        }
        glVertex2f(displayWidth(), gridSize * skippedStart);
      glEnd();
      skippedStart = -1.0;
    }
    glBegin(GL_TRIANGLE_STRIP); // Paint initiative two vertexes of the strip line..
//       glColor3f(1.0f, 1.0f, 1.0f);
      glTexCoord2f(xSize * lastXMoves[0], -ySize * (y + 1.0f + lastYMoves[0])); // Left bottom
      glVertex2f(0.0f, y*gridSize + gridSize);
      glTexCoord2f(xSize * lastlineXMoves[0], -ySize * (y + lastlineYMoves[0])); // Left top
      glVertex2f(0.0f, y*gridSize);

      for(float x = 0.0f; x < xCount; x++) {
        const int ix = x;

        lastHeights[ix + 1] = getHeight(x*gridSize, y*gridSize);
        lastXMoves[ix + 1] = (lastHeights[ix + 1] - lastHeights[ix])/-gridSize;
        lastYMoves[ix + 1] = (lastHeights[ix] - lastlineHeights[ix])/gridSize;

        glTexCoord2f(xSize * (x + 1.0f + lastXMoves[ix + 1]), -ySize * (y + 1.0f + lastYMoves[ix + 1])); // Right bottom
// float xxx = (lastXMoves[ix + 1] + lastYMoves[ix + 1])/20.0f;
//         glColor3f(xxx, xxx, xxx);
        glVertex2f(x*gridSize + gridSize, y*gridSize + gridSize);
        glTexCoord2f(xSize * (x + 1.0f + lastlineXMoves[ix + 1]), -ySize * (y + lastlineYMoves[ix + 1])); // Right top
// xxx = (lastlineXMoves[ix + 1] + lastlineYMoves[ix + 1])/20.0f;
//         glColor3f(xxx, xxx, xxx);
        glVertex2f(x*gridSize + gridSize, y*gridSize);
      }
    glEnd();
    float* temp;
    temp = lastlineHeights; lastlineHeights = lastHeights; lastHeights = temp;
    temp = lastlineXMoves; lastlineXMoves = lastXMoves; lastXMoves = temp;
    temp = lastlineYMoves; lastlineYMoves = lastYMoves; lastYMoves = temp;
  }

//   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

  texture->unbind();
  effects->addRepaintFull();
}

void WaterEffect::mouseChanged(const QPoint &pos, const QPoint &oldpos, Qt::MouseButtons buttons, Qt::MouseButtons oldbuttons, Qt::KeyboardModifiers modifiers, Qt::KeyboardModifiers oldmodifiers)
{
  if(!valid || !mouse || !modifiers.testFlag(Qt::ControlModifier)) return;
  float px = float(pos.x() - gridSize / 2);
  float py = float(pos.y() - gridSize / 2);
//   if(!buttons.testFlag(Qt::LeftButton)) {
//     waves.push_back(new CircleWave(px, py, 40.0f, 80.0f/(2.0f*3.14f), 10.0f, 50.0f, 120.0f, time, 0.25f, 0));
//   }
  if(buttons.testFlag(Qt::LeftButton)) {
    if(oldbuttons.testFlag(Qt::LeftButton)) {
      float tx = pos.x() - oldpos.x();
      float ty = pos.y() - oldpos.y();
      float length = sqrt(tx*tx + ty*ty);
      float count = 1.0f;
      if(length > 20.0f) count += 2.0f;
      if(length > 40.0f) count += 2.0f;
      waves.push_back(new CircleWave(px, py, defaultAmplitude*0.75, defaultPeriodeLength, defaultDamping, defaultRadius, defaultPeriodeLength*3.14f*count, time, defaultSpeed));
    } else
      waves.push_back(new CircleWave(px, py, defaultAmplitude,      defaultPeriodeLength, defaultDamping, defaultRadius, defaultPeriodeLength*3.0f*3.14f,  time, defaultSpeed));
  }
}

// void WaterEffect::windowUnminimized(EffectWindow* w) {
//     roundedRectangle(float(w->x()) - gridSize, float(w->y()) - gridSize, float(w->x() + w->width()) + gridSize, float(w->y() + w->height()) + gridSize);
// }

void WaterEffect::windowActivated(EffectWindow* w) {
    if(activateWave && w) roundedRectangle(float(w->x()) - gridSize, float(w->y()) - gridSize, float(w->x() + w->width()) + gridSize, float(w->y() + w->height()) + gridSize);
}

void WaterEffect::roundedRectangle(float left, float top, float right, float bottom) {
    waves.push_back(new CircleWave(right, top,    defaultAmplitude, defaultPeriodeLength, defaultDamping, defaultRadius, defaultPeriodeLength*3.0f*3.14f, time, defaultSpeed, 1));
    waves.push_back(new CircleWave(right, bottom, defaultAmplitude, defaultPeriodeLength, defaultDamping, defaultRadius, defaultPeriodeLength*3.0f*3.14f, time, defaultSpeed, 2));
    waves.push_back(new CircleWave(left,  bottom, defaultAmplitude, defaultPeriodeLength, defaultDamping, defaultRadius, defaultPeriodeLength*3.0f*3.14f, time, defaultSpeed, 3));
    waves.push_back(new CircleWave(left,  top,    defaultAmplitude, defaultPeriodeLength, defaultDamping, defaultRadius, defaultPeriodeLength*3.0f*3.14f, time, defaultSpeed, 4));
    waves.push_back(new LineWave(  left,  top,    defaultAmplitude, defaultPeriodeLength, defaultDamping, defaultRadius, defaultPeriodeLength*3.0f*3.14f, time, defaultSpeed, 1, bottom-top));
    waves.push_back(new LineWave(  left,  top,    defaultAmplitude, defaultPeriodeLength, defaultDamping, defaultRadius, defaultPeriodeLength*3.0f*3.14f, time, defaultSpeed, 2, right-left));
    waves.push_back(new LineWave(  right, top,    defaultAmplitude, defaultPeriodeLength, defaultDamping, defaultRadius, defaultPeriodeLength*3.0f*3.14f, time, defaultSpeed, 3, bottom-top));
    waves.push_back(new LineWave(  left,  bottom, defaultAmplitude, defaultPeriodeLength, defaultDamping, defaultRadius, defaultPeriodeLength*3.0f*3.14f, time, defaultSpeed, 4, right-left));
}

void WaterEffect::letRain() {
  if(time - rainLastDrop > rainDelay) {
    waves.push_back(new CircleWave(float(rand() % displayWidth()), float(rand() % displayHeight()), 30.0f, 80.0f/(2.0f*3.14f), 60.0f, float(rand() % 100 + 120), 40.0f, time, 0.25f, 0));
    rainLastDrop = time;
  }
}

// CircleWave class:

float WaterEffect::CircleWave::getHeight(float p_x, float p_y, float time)
{
  float t1 = p_x-x;
  float t2 = p_y-y;
  if(kvadrant == 0 ||
     (kvadrant == 1 && t1 > 0.0f && t2 < 0.0f) ||
     (kvadrant == 2 && t1 > 0.0f && t2 > 0.0f) ||
     (kvadrant == 3 && t1 < 0.0f && t2 > 0.0f) ||
     (kvadrant == 4 && t1 < 0.0f && t2 < 0.0f))
  {
    float length = sqrt(t1*t1+t2*t2);
    if(length > radius) // If this point is outside of our radius, return 0..
      return 0.0f;
    float startDistance = (time-birthTime) * speed; // How far is start of wave..
    if(startDistance < length || startDistance - periods > length) // If wave isn't yet or no longer in this point, return 0..
      return 0.0f;
    float height = -sin((length - startDistance) / periodeLength) * amplitude;
    if(length <= damping) // This point is still in area whitout damping..
      return height;
    return height * (1.0f - (length - damping) / (radius - damping));
  }
  return 0.0f;
}

bool WaterEffect::CircleWave::isIn(float p_y, float time)
{
  return p_y >= y - radius && p_y <= y + radius;
}

bool WaterEffect::CircleWave::isReadyToDelete(float time)
{
  return (time-birthTime) * speed > periods + radius;
}

// LineWave class:

float WaterEffect::LineWave::getHeight(float p_x, float p_y, float time)
{
  float length = 0.0f;
  switch(kvadrant) {
    case 1:
      length = x-p_x;
      if(y > p_y || y < p_y - size) return 0.0f;
      break;
    case 2:
      length = y-p_y;
      if(x > p_x || x < p_x - size) return 0.0f;
      break;
    case 3:
      length = p_x-x;
      if(y > p_y || y < p_y - size) return 0.0f;
      break;
    case 4:
      length = p_y-y;
      if(x > p_x || x < p_x - size) return 0.0f;
      break;
  }
  if(length > radius || length < 0.0f)
   return 0.0f;
  float startDistance = (time-birthTime) * speed; // How far is start of wave..
  if(startDistance < length || startDistance - periods > length) // If wave isn't yet or no longer in this point, return 0..
    return 0.0f;
  float height = -sin((length - startDistance) / periodeLength) * amplitude;
  if(length <= damping) // This point is in area where isn't resistance..
    return height;
  return height * (1.0f - (length - damping) / (radius - damping));
}

bool WaterEffect::LineWave::isIn(float p_y, float time)
{
  switch(kvadrant) {
    case 1:
    case 3:
      if(y < p_y && y > p_y - size) return true;
      break;
    case 2:
      if(y >= p_y - periodeLength*3.14f && y <= p_y + radius) return true;
      break;
    case 4:
      if(y <= p_y + periodeLength*3.14f && y >= p_y - radius) return true;
      break;
  }
  return false;
}

bool WaterEffect::LineWave::isReadyToDelete(float time)
{
  return (time-birthTime) * speed > periods + radius;
}

} // namespace

#include "watereffect.moc"
