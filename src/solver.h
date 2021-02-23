/***********************************************************************
 *
 * Copyright (C) 2009, 2010, 2011 Graeme Gott <graeme@gottcode.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ***********************************************************************/

#ifndef SOLVER_H
#define SOLVER_H

#include "trie.h"

#include <QHash>
#include <QPoint>
#include <QVector>

class Solver
{
public:
	Solver(const Trie& words, int size, int minimum);

	void solve(const QStringList& letters);

	int count() const
	{
		return m_count;
	}

	QHash<QString, QList<QList<QPoint>>> solutions() const
	{
		return m_solutions;
	}

	int score(int max = -1) const;
	static int score(const QString& word);

	void setTrackPositions(bool track_positions);

private:
	struct Cell;
	void checkCell(Cell& cell);

private:
	const Trie* m_words;
	const Trie::Node* m_node;
	int m_size;
	int m_minimum;
	bool m_track_positions;
	QString m_word;
	QList<QPoint> m_positions;
	QHash<QString, QList<QList<QPoint>>> m_solutions;
	int m_count;

	struct Cell
	{
		QString text;
		QVector<Cell*> neighbors;
		QPoint position;
		bool checked;
	};
	QVector<QVector<Cell>> m_cells;
};

#endif
