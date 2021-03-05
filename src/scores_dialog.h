/*
	SPDX-FileCopyrightText: 2009-2021 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TANGLET_SCORES_DIALOG_H
#define TANGLET_SCORES_DIALOG_H

#include <QDateTime>
#include <QDialog>
class QAbstractButton;
class QDialogButtonBox;
class QGridLayout;
class QLabel;
class QLineEdit;

/**
 * @brief The ScoresDialog class displays the list of high scores.
 */
class ScoresDialog : public QDialog
{
	Q_OBJECT

	/**
	 * @brief The ScoresDialog::Score struct descibres a high score.
	 */
	struct Score
	{
		QString name; /**< the player's name */
		int score; /**< the value of the score */
		int max_score; /**< the maximum score available on the played board */
		QDateTime date; /**< when the score was made */
		int size; /**< the size of the board */

		/**
		 * Less than comparison used to sort scores.
		 * @param s score to compare with
		 * @return if this score is smaller
		 */
		bool operator<(const Score& s) const
		{
			return score < s.score;
		}
	};

public:
	/**
	 * Constructs a scores dialog.
	 * @param parent the QWidget that manages the dialog
	 */
	explicit ScoresDialog(QWidget* parent = nullptr);

	/**
	 * Attempts to add a score.
	 * @param score the value of the score
	 * @param max_score the maximum score available on the played board
	 * @return whether the score was added
	 */
	bool addScore(int score, int max_score);

	/**
	 * Checks if a score is a high score.
	 * @param score the value of the score
	 * @param timer the timer mode to check for scores
	 * @return whether the score is a high score, and if it is the highest
	 */
	static int isHighScore(int score, int timer);

	/**
	 * Converts the stored scores to the new format.
	 */
	static void migrate();

signals:
	/**
	 * The high score list has been cleared.
	 */
	void scoresReset();

protected:
	/**
	 * Override hideEvent to add score if the player has not already pressed enter.
	 * @param event details of the hide event
	 */
	void hideEvent(QHideEvent* event) override;

	/**
	 * Override keyPressEvent to focus the close button; dialog only recieves these events if the
	 * line edit is not focused.
	 * @param event details of the key press event
	 */
	void keyPressEvent(QKeyEvent* event) override;

private slots:
	/**
	 * Enters the score and saves list of scores once the player has finished entering their name.
	 */
	void editingFinished();

	/**
	 * Resets the board if the player has activated the reset button.
	 * @param button which button the player activated
	 */
	void resetClicked(QAbstractButton* button);

private:
	/**
	 * Adds a score to the high score board.
	 * @param name the player's name
	 * @param score the value of the score
	 * @param max_score the maximum score available on the played board
	 * @param date when the score was made
	 * @param timer the active timer mode
	 * @param size the size of the board
	 * @return the location in the list of scores or -1 if not a high score
	 */
	int addScore(const QString& name, int score, int max_score, const QDateTime& date, int timer, int size);

	/**
	 * Loads the scores from the settings.
	 */
	void load();

	/**
	 * Sets the text of the high scores. Adds the dashed lines for empty scores.
	 */
	void updateItems();

private:
	QDialogButtonBox* m_buttons; /**< buttons to control dialog */

	QList<Score> m_scores; /**< the high score data */
	QString m_default_name; /**< the default name */

	QLabel* m_score_labels[10][6]; /**< the grid[row][column] of labels to display the scores */
	QGridLayout* m_scores_layout; /**< the layout for the dialog */
	QLineEdit* m_username; /**< widget for the player to enter their name */
	int m_row; /**< location of most recently added score */

	static QVector<int> m_max; /**< the largest high score */
	static QVector<int> m_min; /**< the smallest high score */
};

#endif // TANGLET_SCORES_DIALOG_H
