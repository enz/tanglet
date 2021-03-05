/*
	SPDX-FileCopyrightText: 2009-2021 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "scores_dialog.h"

#include "board.h"
#include "clock.h"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QStyle>
#include <QVBoxLayout>

#if defined(Q_OS_UNIX)
#include <pwd.h>
#include <unistd.h>
#elif defined(Q_OS_WIN)
#include <lmcons.h>
#include <windows.h>
#endif

//-----------------------------------------------------------------------------

int ScoresDialog::m_max = -1;
int ScoresDialog::m_min = 1;

//-----------------------------------------------------------------------------

ScoresDialog::ScoresDialog(QWidget* parent)
	: QDialog(parent)
	, m_row(-1)
{
	setWindowTitle(tr("High Scores"));

	QSettings settings;

	// Load default name
	m_default_name = settings.value("Scores/DefaultName").toString();
	if (m_default_name.isEmpty()) {
#if defined(Q_OS_UNIX)
		passwd* pws = getpwuid(geteuid());
		if (pws) {
			m_default_name = QString::fromLocal8Bit(pws->pw_gecos).section(',', 0, 0);
			if (m_default_name.isEmpty()) {
				m_default_name = QString::fromLocal8Bit(pws->pw_name);
			}
		}
#elif defined(Q_OS_WIN)
		TCHAR buffer[UNLEN + 1];
		DWORD count = UNLEN + 1;
		if (GetUserName(buffer, &count)) {
			m_default_name = QString::fromWCharArray(buffer, count);
		}
#endif
	}

	// Create score widgets
	m_scores_layout = new QGridLayout;
	m_scores_layout->setSpacing(12);
	m_scores_layout->setColumnStretch(1, 1);
	m_scores_layout->addWidget(new QLabel(tr("<b>Name</b>"), this), 0, 1, Qt::AlignCenter);
	m_scores_layout->addWidget(new QLabel(tr("<b>Score</b>"), this), 0, 2, Qt::AlignCenter);
	m_scores_layout->addWidget(new QLabel(tr("<b>Maximum</b>"), this), 0, 3, Qt::AlignCenter);
	m_scores_layout->addWidget(new QLabel(tr("<b>Date</b>"), this), 0, 4, Qt::AlignCenter);
	m_scores_layout->addWidget(new QLabel(tr("<b>Size</b>"), this), 0, 5, Qt::AlignCenter);

	Qt::Alignment alignments[6] = {
		Qt::AlignRight,
		Qt::AlignLeft,
		Qt::AlignRight,
		Qt::AlignRight,
		Qt::AlignRight,
		Qt::AlignHCenter
	};
	for (int r = 0; r < 10; ++r) {
		m_score_labels[r][0] = new QLabel(tr("#%1").arg(r + 1), this);
		m_scores_layout->addWidget(m_score_labels[r][0], r + 1, 0, alignments[0] | Qt::AlignVCenter);
		for (int c = 1; c < 6; ++c) {
			m_score_labels[r][c] = new QLabel("-", this);
			m_scores_layout->addWidget(m_score_labels[r][c], r + 1, c, alignments[c] | Qt::AlignVCenter);
		}
	}

	load();

	m_username = new QLineEdit(this);
	m_username->hide();
	connect(m_username, &QLineEdit::editingFinished, this, &ScoresDialog::editingFinished);

	// Hide maximum scores column if showing maximum scores is set to "Never"
	if (settings.value("ShowMaximumScore").toInt() == 0) {
		m_scores_layout->itemAtPosition(0, 3)->widget()->hide();
		for (int r = 0; r < 10; ++r) {
			m_score_labels[r][3]->hide();
		}
	}

	// Lay out dialog
	m_buttons = new QDialogButtonBox(QDialogButtonBox::Reset | QDialogButtonBox::Close, Qt::Horizontal, this);
	m_buttons->setCenterButtons(style()->styleHint(QStyle::SH_MessageBox_CenterButtons));
	m_buttons->button(QDialogButtonBox::Reset)->setAutoDefault(false);
	m_buttons->button(QDialogButtonBox::Close)->setDefault(true);
	m_buttons->button(QDialogButtonBox::Close)->setFocus();
	connect(m_buttons, &QDialogButtonBox::rejected, this, &ScoresDialog::reject);
	connect(m_buttons, &QDialogButtonBox::clicked, this, &ScoresDialog::resetClicked);

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addLayout(m_scores_layout);
	layout->addWidget(m_buttons);
}

//-----------------------------------------------------------------------------

bool ScoresDialog::addScore(int score, int max_score)
{
	// Add score
	QSettings settings;
	m_row = addScore(m_default_name,
			score,
			max_score,
			QDateTime::currentDateTime(),
			settings.value("Current/TimerMode", Clock::Tanglet).toInt(),
			settings.value("Current/Size", 4).toInt());
	if (m_row == -1) {
		return false;
	}
	for (int c = 0; c < 6; ++c) {
		QFont f = m_score_labels[m_row][c]->font();
		f.setWeight(QFont::Bold);
		m_score_labels[m_row][c]->setFont(f);
	}
	updateItems();

	// Show lineedit
	m_scores_layout->addWidget(m_username, m_row + 1, 1);
	m_score_labels[m_row][1]->hide();
	m_username->setText(m_default_name);
	m_username->show();
	m_username->setFocus();

	m_buttons->button(QDialogButtonBox::Close)->setDefault(false);

	return true;
}

//-----------------------------------------------------------------------------

int ScoresDialog::isHighScore(int score)
{
	if (m_max == -1) {
		m_max = 1;
		ScoresDialog();
	}

	if (score >= m_max) {
		return 2;
	} else if (score >= m_min) {
		return 1;
	} else {
		return 0;
	}
}

//-----------------------------------------------------------------------------

void ScoresDialog::migrate()
{
	QSettings settings;
	if (!settings.contains("Scores/Values")) {
		return;
	}

	const QStringList data = settings.value("Scores/Values").toStringList();
	settings.remove("Scores/Values");

	QVector<int> indexes(Clock::TotalTimers, 0);

	for (const QString& s : data) {
		const QStringList values = s.split(':');
		if (values.size() < 3 || values.size() > 6) {
			continue;
		}

		const QString name = values[0];
		const int score = values[1].toInt();
		const int max_score = values.value(4, "-1").toInt();
		const QDateTime date = QDateTime::fromString(values[2], "yyyy.MM.dd-hh.mm.ss");
		const int timer = values.value(3, QString::number(Clock::Tanglet)).toInt();
		const int size = values.value(5, "-1").toInt();

		int& index = indexes[timer];
		settings.beginWriteArray(Clock::timerScoresGroup(timer));
		settings.setArrayIndex(index);
		settings.setValue("Name", name);
		settings.setValue("Score", score);
		settings.setValue("Maximum", max_score);
		settings.setValue("Size", size);
		settings.setValue("Date", date.toString(Qt::ISODate));
		settings.endArray();
		++index;
	}
}

//-----------------------------------------------------------------------------

void ScoresDialog::hideEvent(QHideEvent* event)
{
	if (m_username->isVisible()) {
		editingFinished();
	}
	QDialog::hideEvent(event);
}

//-----------------------------------------------------------------------------

void ScoresDialog::keyPressEvent(QKeyEvent* event)
{
	if (!m_buttons->button(QDialogButtonBox::Close)->isDefault()) {
		m_buttons->button(QDialogButtonBox::Close)->setDefault(true);
		m_buttons->button(QDialogButtonBox::Close)->setFocus();
		event->ignore();
		return;
	}
	QDialog::keyPressEvent(event);
}

//-----------------------------------------------------------------------------

void ScoresDialog::editingFinished()
{
	// Hide lineedit
	m_username->hide();
	m_scores_layout->removeWidget(m_username);
	m_scores[m_row].name = m_username->text();
	m_score_labels[m_row][1]->show();
	updateItems();

	// Save scores
	QSettings settings;
	settings.setValue("Scores/DefaultName", m_username->text());
	const int timer = settings.value("Current/TimerMode", Clock::Tanglet).toInt();
	settings.beginWriteArray(Clock::timerScoresGroup(timer));
	for (int r = 0, size = m_scores.size(); r < size; ++r) {
		const Score& score = m_scores[r];
		settings.setArrayIndex(r);
		settings.setValue("Name", score.name);
		settings.setValue("Score", score.score);
		settings.setValue("Maximum", score.max_score);
		settings.setValue("Size", score.size);
		settings.setValue("Date", score.date.toString(Qt::ISODate));
	}
	settings.endArray();
}

//-----------------------------------------------------------------------------

void ScoresDialog::resetClicked(QAbstractButton* button)
{
	if (m_buttons->buttonRole(button) == QDialogButtonBox::ResetRole) {
		if (QMessageBox::question(this, tr("Question"), tr("Clear high scores?"), QMessageBox::No | QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
			if (m_username->isVisible()) {
				editingFinished();
			}
			m_scores.clear();
			m_max = m_min = 1;
			if (m_row > -1) {
				for (int c = 0; c < 6; ++c) {
					QFont f = m_score_labels[m_row][c]->font();
					f.setWeight(QFont::Normal);
					m_score_labels[m_row][c]->setFont(f);
				}
			}
			updateItems();
			QSettings settings;
			for (int timer = 0; timer < Clock::TotalTimers; ++timer) {
				settings.remove(Clock::timerScoresGroup(timer));
			}
			emit scoresReset();
		}
	}
}

//-----------------------------------------------------------------------------

int ScoresDialog::addScore(const QString& name, int score, int max_score, const QDateTime& date, int timer, int size)
{
	if (score == 0) {
		return -1;
	}

	int row = 0;
	for (const Score& s : qAsConst(m_scores)) {
		if (score >= s.score && date >= s.date) {
			break;
		}
		row++;
	}
	if (row == 10) {
		return -1;
	}

	Score s = { name, score, max_score, date, size };
	m_scores.insert(row, s);
	if (m_scores.count() == 11) {
		m_scores.removeLast();
	}

	m_max = m_scores.first().score;
	m_min = (m_scores.count() == 10) ? m_scores.last().score : 1;

	return row;
}

//-----------------------------------------------------------------------------

void ScoresDialog::load()
{
	const int timer = Clock::Tanglet;
	QSettings settings;
	const int size = std::min(settings.beginReadArray(Clock::timerScoresGroup(timer)), 10);
	for (int r = 0; r < size; ++r) {
		settings.setArrayIndex(r);
		const QString name = settings.value("Name").toString();
		const int score = settings.value("Score").toInt();
		const int max_score = settings.value("Maximum", -1).toInt();
		const int size = settings.value("Size", -1).toInt();
		const QDateTime date = settings.value("Date").toDateTime();
		addScore(name, score, max_score, date, timer, size);
	}
	settings.endArray();
	updateItems();
}

//-----------------------------------------------------------------------------

void ScoresDialog::updateItems()
{
	int count = m_scores.count();
	for (int r = 0; r < count; ++r) {
		const Score& score = m_scores.at(r);
		m_score_labels[r][1]->setText(score.name);
		m_score_labels[r][2]->setNum(score.score);
		if (score.max_score > -1) {
			m_score_labels[r][3]->setNum(score.max_score);
		} else {
			m_score_labels[r][3]->setText(tr("N/A"));
		}
		m_score_labels[r][4]->setText(QLocale().toString(score.date, QLocale::ShortFormat));
		if (score.size > -1) {
			m_score_labels[r][5]->setText(Board::sizeToString(score.size));
		} else {
			m_score_labels[r][5]->setText(tr("N/A"));
		}
	}
	for (int r = count; r < 10; ++r) {
		for (int c = 1; c < 6; ++c) {
			m_score_labels[r][c]->setText("-");
		}
	}
}

//-----------------------------------------------------------------------------
