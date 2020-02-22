/*
 * InstrumentRandomizer.h - tool to randomize instruments
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#include "ToolPlugin.h"
#include "ToolPluginView.h"
#include "Instrument.h"
#include "InstrumentTrack.h"
#include "FileBrowser.h"

class InstrumentRandomizerView : public ToolPluginView
{
	Q_OBJECT
public:
	InstrumentRandomizerView( ToolPlugin * _tool );
	virtual ~InstrumentRandomizerView();
	void randomizeInstrument(InstrumentTrack * track);
	QString getRandomPreset();
	QStringList presets = {};
	long presetMin = 0;
	long presetMax = 1;
	QGridLayout *gridLayout;
	QVBoxLayout *verticalLayout;
	QPushButton *buttonRandomizeActive;
	QPushButton *buttonRandomizeAll;
	QPushButton *buttonRandomizeAllActive;
	QLabel *totalPresets;
	LedCheckBox *cbSF2;
	BoolModel m_SF2;
	LedCheckBox *cbXPF;
	BoolModel m_XPF;
	LedCheckBox *cbXIZ;
	BoolModel m_XIZ;
	LedCheckBox *cbResetVolume;
	BoolModel m_ResetVolume;
	LedCheckBox *cbResetPanning;
	BoolModel m_ResetPanning;
	LedCheckBox *cbClearEffects;
	BoolModel m_ClearEffects;

private slots:
	void findPresets();
	void randomizeActive();
	void randomizeAll();
	void randomizeAllActive();
} ;


class InstrumentRandomizer : public ToolPlugin
{
public:
	InstrumentRandomizer();
	virtual ~InstrumentRandomizer();

	virtual PluginView * instantiateView( QWidget * )
	{
		return new InstrumentRandomizerView( this );
	}

	virtual QString nodeName() const;

	virtual void saveSettings( QDomDocument& doc, QDomElement& element )
	{
		Q_UNUSED(doc)
		Q_UNUSED(element)
	}

	virtual void loadSettings( const QDomElement& element )
	{
		Q_UNUSED(element)
	}


} ;
