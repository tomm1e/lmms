/*
 * InstrumentRandomizer.cpp - tool to randomize instruments
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

#include "InstrumentRandomizer.h"

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include <QDebug>
#include <QDir>
#include <QList>
#include <QMdiArea>
#include <QDirIterator>

#include "AutomatableModel.h"
#include "BBEditor.h"
#include "BBTrackContainer.h"
#include "ConfigManager.h"
#include "EffectChain.h"
#include "Engine.h"
#include "FileBrowser.h"
#include "FxMixerView.h"
#include "gui_templates.h"
#include "GuiApplication.h"
#include "Instrument.h"
#include "InstrumentTrack.h"
#include "LedCheckbox.h"
#include "MainWindow.h"
#include "PluginFactory.h"
#include "Song.h"
#include "Track.h"
#include "SongEditor.h"

#include "embed.h"
#include "plugin_export.h"

extern "C"
{
	Plugin::Descriptor PLUGIN_EXPORT instrumentrandomizer_plugin_descriptor =
		{
			STRINGIFY(PLUGIN_NAME),
			"Instrument Randomizer",
			QT_TRANSLATE_NOOP("pluginBrowser",
							  "Randomize instruments"),
			"Tommie Harper <teh420/at/gmail.com>",
			0x0100,
			Plugin::Tool,
			new PluginPixmapLoader("logo"),
			NULL,
			NULL};

	PLUGIN_EXPORT Plugin *lmms_plugin_main(Model *_parent, void *_data)
	{
		return new InstrumentRandomizer;
	}
}

InstrumentRandomizer::InstrumentRandomizer() : ToolPlugin(&instrumentrandomizer_plugin_descriptor, NULL)
{
	printf("Loaded ToolPlugin.\n");
}

InstrumentRandomizer::~InstrumentRandomizer()
{
	printf("Loaded.\n");
}

QString InstrumentRandomizer::nodeName() const
{
	return instrumentrandomizer_plugin_descriptor.name;
}

InstrumentRandomizerView::~InstrumentRandomizerView()
{
	printf("Loaded View.\n");
}

InstrumentRandomizerView::InstrumentRandomizerView(ToolPlugin *_tool) : ToolPluginView(_tool)
{
	this->setWindowTitle("Instrument Randomizer");

	if (this->objectName().isEmpty())
		this->setObjectName(QString::fromUtf8("Form"));

	QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	sizePolicy.setHorizontalStretch(0);
	sizePolicy.setVerticalStretch(0);
	sizePolicy.setHeightForWidth(this->sizePolicy().hasHeightForWidth());
	this->setSizePolicy(sizePolicy);

	gridLayout = new QGridLayout(this);
	gridLayout->setObjectName(QString::fromUtf8("gridLayout"));

	verticalLayout = new QVBoxLayout();
	verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));

	totalPresets = new QLabel(this);
	totalPresets->setObjectName(QString::fromUtf8("totalPresets"));
	totalPresets->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
	totalPresets->setText("Presets Found: 0");
	verticalLayout->addWidget(totalPresets);

	cbSF2 = new LedCheckBox("", this);
	cbSF2->setObjectName(QString::fromUtf8("cbSF2"));
	cbSF2->setModel(&m_SF2);
	cbSF2->setText("Include SF2");
	verticalLayout->addWidget(cbSF2);

	cbXPF = new LedCheckBox("", this);
	cbXPF->setObjectName(QString::fromUtf8("cbXPF"));
	cbXPF->setModel(&m_XPF);
	cbXPF->setText("Include XPF");
	verticalLayout->addWidget(cbXPF);

	cbXIZ = new LedCheckBox("", this);
	cbXIZ->setObjectName(QString::fromUtf8("cbXIZ"));
	cbXIZ->setModel(&m_XIZ);
	cbXIZ->setChecked(true);
	cbXIZ->setText("Include XIZ");
	verticalLayout->addWidget(cbXIZ);

	cbClearEffects = new LedCheckBox("", this);
	cbClearEffects->setObjectName(QString::fromUtf8("cbClearEffects"));
	cbClearEffects->setModel(&m_ClearEffects);
	cbClearEffects->setChecked(true);
	cbClearEffects->setText("Clear Effects Chain");
	verticalLayout->addWidget(cbClearEffects);

	cbResetVolume = new LedCheckBox("", this);
	cbResetVolume->setObjectName(QString::fromUtf8("cbResetVolume"));
	cbResetVolume->setModel(&m_ResetVolume);
	cbResetVolume->setChecked(true);
	cbResetVolume->setText("Reset Volume");
	verticalLayout->addWidget(cbResetVolume);

	cbResetPanning = new LedCheckBox("", this);
	cbResetPanning->setObjectName(QString::fromUtf8("cbResetPanning"));
	cbResetPanning->setModel(&m_ResetPanning);
	cbResetPanning->setChecked(true);
	cbResetPanning->setText("Reset Panning");
	verticalLayout->addWidget(cbResetPanning);

	cbSkipStandard = new LedCheckBox("", this);
	cbSkipStandard->setObjectName(QString::fromUtf8("cbSkipStandard"));
	cbSkipStandard->setModel(&m_SkipStandard);
	cbSkipStandard->setChecked(true);
	cbSkipStandard->setText("Skip Standard Bank");
	verticalLayout->addWidget(cbSkipStandard);

	buttonRandomizeActive = new QPushButton(this);
	buttonRandomizeActive->setObjectName(QString::fromUtf8("buttonRandomizeActive"));
	buttonRandomizeActive->setText("Randomize Active");
	verticalLayout->addWidget(buttonRandomizeActive);

	buttonRandomizeAll = new QPushButton(this);
	buttonRandomizeAll->setObjectName(QString::fromUtf8("buttonRandomizeAll"));
	buttonRandomizeAll->setText("Randomize All");
	verticalLayout->addWidget(buttonRandomizeAll);

	buttonRandomizeAllActive = new QPushButton(this);
	buttonRandomizeAllActive->setObjectName(QString::fromUtf8("buttonRandomizeAllActive"));
	buttonRandomizeAllActive->setText("Randomize All Active");
	verticalLayout->addWidget(buttonRandomizeAllActive);

	buttonCleanupProject = new QPushButton(this);
	buttonCleanupProject->setObjectName(QString::fromUtf8("buttonCleanupProject"));
	buttonCleanupProject->setText("Remove Automation Tracks");
	verticalLayout->addWidget(buttonCleanupProject);

	buttonRenameTracks = new QPushButton(this);
	buttonRenameTracks->setObjectName(QString::fromUtf8("buttonRenameTracks"));
	buttonRenameTracks->setText("Rename SF2 Tracks");
	verticalLayout->addWidget(buttonRenameTracks);

	gridLayout->addLayout(verticalLayout, 0, 0, 1, 1);

	connect(cbSF2, SIGNAL(clicked()), this, SLOT(findPresets()));
	connect(cbXPF, SIGNAL(clicked()), this, SLOT(findPresets()));
	connect(cbXIZ, SIGNAL(clicked()), this, SLOT(findPresets()));

	connect(buttonRandomizeActive, SIGNAL(clicked()), this, SLOT(randomizeActive()));
	connect(buttonRandomizeAll, SIGNAL(clicked()), this, SLOT(randomizeAll()));
	connect(buttonRandomizeAllActive, SIGNAL(clicked()), this, SLOT(randomizeAllActive()));
	connect(buttonCleanupProject, SIGNAL(clicked()), this, SLOT(cleanupProject()));
	connect(buttonRenameTracks, SIGNAL(clicked()), this, SLOT(renameTracks()));

	if( parentWidget() )
	{
		// parentWidget()->hide();
		parentWidget()->layout()->setSizeConstraint(
							QLayout::SetMinimumSize );

		parentWidget()->move(0, 0);

		// Qt::WindowFlags flags = parentWidget()->windowFlags();
		// flags |= Qt::MSWindowsFixedSizeDialogHint;
		// flags &= ~Qt::WindowMaximizeButtonHint;
		// parentWidget()->setWindowFlags( flags );

		setMinimumSize(250, 290);
	}

	findPresets();
}

void InstrumentRandomizerView::findPresets()
{
	const QString dir = ConfigManager::inst()->factoryPresetsDir();
	// const QString dir2 = ConfigManager::inst()->userPresetsDir();
	presets.clear();

	QDirIterator it(dir, QDirIterator::Subdirectories);
	while (it.hasNext()) {
		QString n = it.next();
		if (m_XIZ.value() && n.endsWith("xiz")) {
			presets << n;
		}
		if (m_XPF.value() && n.endsWith("xpf")) {
			presets << n;
		}
	}

	presetMin = 0;
	presetMax = presets.length();

	totalPresets->setText(QString("Presets Found: ").append(QString::number(presetMax)));
}

QString InstrumentRandomizerView::getRandomPreset()
{
	int randNum = rand() % ((presetMax - presetMin) + 1) + presetMin;
	QString randPreset = presets.value(randNum);
	return (randPreset == NULL) ? "" : randPreset;
}

void InstrumentRandomizerView::randomizeInstrument(InstrumentTrack * track)
{
	const QString randomPreset(getRandomPreset());

	// printf("Preset = %s\n", randomPreset.toStdString().c_str());
	if (randomPreset.isEmpty() && !m_SF2.value()) {
		printf("No variables.\n");
		return;
	}

	const QFileInfo fi(randomPreset);
	const QString ext = fi.completeSuffix();

	InstrumentTrack * it = track;
	Instrument * i = track->instrument();

	if (m_SkipStandard.value()) {
		if (i->nodeName() == "sf2player") {
			int bankValue = i->childModel("info")->property("bank_value").toInt();
			if (bankValue == 128) {
				printf("Skipping Standard Bank\n");
				return;
			}
		}
	}

	if (m_ClearEffects.value()) it->audioPort()->effects()->clear();
	if (m_ResetVolume.value()) it->setVolume(100);
	if (m_ResetPanning.value()) it->panningModel()->setValue(0.0);
	//if (m_ResetPitch.value()) it->pitchModel()->setValue(0.0);

	int r = rand() % 100;
	if (((r > 50) && m_SF2.value()) || (m_SF2.value() && randomPreset.isEmpty())) {
		i = it->loadInstrument( "sf2player" );
		if (i) {
			i->loadFile( ConfigManager::inst()->sf2File() );

			int patchMaxValue = i->childModel("info")->property("patch_max_value").toInt();

			int randPatch = rand()%patchMaxValue;
			int randBank = 0;

			// chance of standard bank
			if ((rand() % 100) > 90) {
				randBank = 128;
				randPatch = 0;
			}

			i->childModel( "bank" )->setValue( randBank );
			i->childModel( "patch" )->setValue( randPatch );

			it->setName(i->childModel("info")->property("current_patch_name").toString());
		}
	} else {
		if (ext == "xiz") {
			if( i == NULL || !i->descriptor()->supportsFileType( ext ) ) {
				PluginFactory::PluginInfoAndKey piakn = pluginFactory->pluginSupportingExtension(ext);
				i = it->loadInstrument(piakn.info.name(), &piakn.key);
			}
			i->loadFile(randomPreset);
		} else if (ext == "xpf") {
			DataFile dataFile( randomPreset );
			InstrumentTrack::removeMidiPortNode( dataFile );
			it->setSimpleSerializing();
			it->loadSettings( dataFile.content().toElement() );
		}
	}
}

void InstrumentRandomizerView::randomizeActive()
{
	QList<QMdiSubWindow*> pl = gui->mainWindow()->workspace()->subWindowList( QMdiArea::StackingOrder );
	QListIterator<QMdiSubWindow *> w( pl );
	w.toBack();
	while( w.hasPrevious() )
	{
		InstrumentTrackWindow * itw = dynamic_cast<InstrumentTrackWindow*>(w.previous()->widget() );
		if( itw != NULL && itw->isHidden() == false )
		{
			randomizeInstrument(itw->model());
			break;
		}
	}
}

void InstrumentRandomizerView::randomizeAll()
{
	TrackContainer::TrackList tracks;

	tracks += Engine::getSong()->tracks();
	tracks += Engine::getBBTrackContainer()->tracks();

	for (const Track* track : tracks) {
		if (track->type() == Track::InstrumentTrack) {
			InstrumentTrack * inst = (InstrumentTrack *) track;
			randomizeInstrument(inst);
		}
	}
}

void InstrumentRandomizerView::randomizeAllActive()
{
	QList<QMdiSubWindow*> pl = gui->mainWindow()->workspace()->subWindowList( QMdiArea::StackingOrder );
	QListIterator<QMdiSubWindow *> w( pl );
	w.toBack();
	while( w.hasPrevious() )
	{
		InstrumentTrackWindow * itw = dynamic_cast<InstrumentTrackWindow*>(w.previous()->widget() );
		if( itw != NULL && itw->isHidden() == false )
		{
			randomizeInstrument(itw->model());
		}
	}
}

void InstrumentRandomizerView::cleanupProject()
{
	if (gui && gui->songEditor()) {
		QList<TrackView *> tvs = gui->songEditor()->m_editor->trackViews();
		for (TrackView* tv : tvs) {
			Track* track = tv->getTrack();
			if (track->type() == Track::AutomationTrack) {
				printf("Removing Automation Track: %s\n", track->fullDisplayName().toStdString().c_str());
				gui->songEditor()->m_editor->deleteTrackView(tv);
			}
		}
	}
}

void InstrumentRandomizerView::renameTracks()
{
	TrackContainer::TrackList tracks;

	tracks += Engine::getSong()->tracks();
	tracks += Engine::getBBTrackContainer()->tracks();

	for (const Track* track : tracks) {
		if (track->type() == Track::InstrumentTrack) {
			InstrumentTrack * it = (InstrumentTrack *) track;
			Instrument * i = it->instrument();
			if (i->nodeName() == "sf2player") {
				it->setName(i->childModel("info")->property("current_patch_name").toString());
			}
		}
	}
}
