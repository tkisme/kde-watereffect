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

#include "watereffect_config.h"

#include <kwineffects.h>

#include <klocale.h>
#include <kdebug.h>
#include <kconfiggroup.h>
#include <KActionCollection>
#include <kaction.h>
#include <KShortcutsEditor>

#include <QWidget>
#include <QVBoxLayout>

#ifndef KDE_USE_FINAL
KWIN_EFFECT_CONFIG_FACTORY
#endif
K_PLUGIN_FACTORY_DEFINITION(EffectFactory, registerPlugin<KWin::WaterEffectConfig> ("watereffect"); )
K_EXPORT_PLUGIN(EffectFactory("kwin"))


namespace KWin
{

WaterEffectConfigForm::WaterEffectConfigForm(QWidget* parent) : QWidget(parent)
{
    setupUi(this);
}

WaterEffectConfig::WaterEffectConfig(QWidget* parent, const QVariantList& args) :
        KCModule(EffectFactory::componentData(), parent, args)
{
    m_ui = new WaterEffectConfigForm(this);

    QVBoxLayout* layout = new QVBoxLayout(this);

    layout->addWidget(m_ui);

    connect(m_ui->editor, SIGNAL(keyChange()), this, SLOT(changed()));
    connect(m_ui->gridSizeSpin, SIGNAL(valueChanged(int)), this, SLOT(changed()));
    connect(m_ui->amplitudeSpin, SIGNAL(valueChanged(int)), this, SLOT(changed()));
    connect(m_ui->periodeSpin, SIGNAL(valueChanged(int)), this, SLOT(changed()));
    connect(m_ui->speedSpin, SIGNAL(valueChanged(int)), this, SLOT(changed()));
    connect(m_ui->dampingSpin, SIGNAL(valueChanged(int)), this, SLOT(changed()));
    connect(m_ui->radiusSpin, SIGNAL(valueChanged(int)), this, SLOT(changed()));
    connect(m_ui->rainDelaySpin, SIGNAL(valueChanged(int)), this, SLOT(changed()));
    connect(m_ui->wavesOnMouseBox, SIGNAL(stateChanged(int)), this, SLOT(changed()));
    connect(m_ui->wavesOnActivationBox, SIGNAL(stateChanged(int)), this, SLOT(changed()));

    m_actionCollection = new KActionCollection( this, componentData() );
    m_actionCollection->setConfigGroup("WaterEffect");
    m_actionCollection->setConfigGlobal(true);

    KAction* a;
    a = static_cast< KAction* >( m_actionCollection->addAction("ToggleRain"));
    a->setText(i18n("Toggle Rain"));
    a->setProperty("isConfigurationAction", true);
    a->setGlobalShortcut(KShortcut(/*Qt::META + Qt::SHIFT + Qt::Key_R*/));

    a = static_cast< KAction* >( m_actionCollection->addAction("EraseWaves"));
    a->setText(i18n("Erase Waves"));
    a->setProperty("isConfigurationAction", true);
    a->setGlobalShortcut(KShortcut(/*Qt::META + Qt::CTRL + Qt::Key_E*/));

    m_ui->editor->addCollection(m_actionCollection);
}

WaterEffectConfig::~WaterEffectConfig()
{
    m_ui->editor->undoChanges();
}

void WaterEffectConfig::load()
{
    KCModule::load();

    KConfigGroup conf = EffectsHandler::effectConfig("WaterEffect");

    m_ui->gridSizeSpin->setValue(conf.readEntry("GridSize", 10));
    m_ui->rainDelaySpin->setValue(conf.readEntry("RainDelay", 30));
    m_ui->wavesOnMouseBox->setChecked(conf.readEntry("Mouse", true));
    m_ui->wavesOnActivationBox->setChecked(conf.readEntry("ActivateWave", false));
    m_ui->amplitudeSpin->setValue(conf.readEntry("defaultAmplitude", 40));
    m_ui->periodeSpin->setValue(conf.readEntry("defaultPeriodeLength", 80));
    m_ui->dampingSpin->setValue(conf.readEntry("defaultDamping", 100));
    m_ui->radiusSpin->setValue(conf.readEntry("defaultRadius", 250));
    m_ui->speedSpin->setValue(conf.readEntry("defaultSpeed", 250));

    emit changed(false);
}

void WaterEffectConfig::save()
{
    KConfigGroup conf = EffectsHandler::effectConfig("WaterEffect");

    conf.writeEntry("GridSize", m_ui->gridSizeSpin->value());
    conf.writeEntry("RainDelay", m_ui->rainDelaySpin->value());
    conf.writeEntry("Mouse", m_ui->wavesOnMouseBox->isChecked());
    conf.writeEntry("ActivateWave", m_ui->wavesOnActivationBox->isChecked());
    conf.writeEntry("defaultAmplitude", m_ui->amplitudeSpin->value());
    conf.writeEntry("defaultPeriodeLength", m_ui->periodeSpin->value());
    conf.writeEntry("defaultDamping", m_ui->dampingSpin->value());
    conf.writeEntry("defaultRadius", m_ui->radiusSpin->value());
    conf.writeEntry("defaultSpeed", m_ui->speedSpin->value());

    m_ui->editor->save();

    conf.sync();

    emit changed(false);
    EffectsHandler::sendReloadMessage("watereffect");
}

void WaterEffectConfig::defaults()
{
    m_ui->gridSizeSpin->setValue(10);
    m_ui->rainDelaySpin->setValue(30);
    m_ui->wavesOnMouseBox->setChecked(true);
    m_ui->wavesOnActivationBox->setChecked(false);
    m_ui->amplitudeSpin->setValue(40);
    m_ui->periodeSpin->setValue(80);
    m_ui->dampingSpin->setValue(100);
    m_ui->radiusSpin->setValue(250);
    m_ui->speedSpin->setValue(250);
    m_ui->editor->allDefault();
    emit changed(true);
}


} // namespace

#include "watereffect_config.moc"
