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

#ifndef KWIN_WATEREFFECT_CONFIG_H
#define KWIN_WATEREFFECT_CONFIG_H

#include <kcmodule.h>

#include "ui_watereffect_config.h"

class KActionCollection;

namespace KWin
{

class WaterEffectConfigForm : public QWidget, public Ui::WaterEffectConfigForm
{
    Q_OBJECT
    public:
        explicit WaterEffectConfigForm(QWidget* parent);
};

class WaterEffectConfig : public KCModule
{
    Q_OBJECT
    public:
        explicit WaterEffectConfig(QWidget* parent = 0, const QVariantList& args = QVariantList());
        virtual ~WaterEffectConfig();

        virtual void save();
        virtual void load();
        virtual void defaults();

    private:
        WaterEffectConfigForm* m_ui;
        KActionCollection* m_actionCollection;
};

} // namespace

#endif
