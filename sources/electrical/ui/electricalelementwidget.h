/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef ELECTRICALELEMENTWIDGET_H
#define ELECTRICALELEMENTWIDGET_H

#include "electrical/devicecatalogue.h"
#include "electrical/electricalmodel.h"
#include "ui/abstractelementpropertieseditorwidget.h"

#include <QMap>
#include <QPointer>

class QComboBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QETProject;

/**
	@brief The "Electrical" tab of the properties of a symbol: what the
	symbol is in the electrical model of the project (a board, the
	protection of a circuit, where circuits start or end), the data of
	its protective device, and the results of the checks of that circuit.

	Its changes go to the project's undo stack as one change of the
	electrical model, like those of the panel. Shown only when the
	"Electrical circuits panel" setting is on.
*/
class ElectricalElementWidget : public AbstractElementPropertiesEditorWidget
{
		Q_OBJECT

	public:
		explicit ElectricalElementWidget(Element *element, QWidget *parent = nullptr);

		void setElement(Element *element) override;
		QString title() const override;
		void apply() override;
		QUndoCommand *associatedUndo() const override;
		void updateUi() override;
		bool setLiveEdit(bool live_edit) override;

			/// The model as the tab would leave it; the project's when unchanged
		electrical::ElectricalModel editedModel() const;
		bool isValid() const;

		QComboBox *boardField() const { return m_board; }
		QComboBox *circuitField() const { return m_circuit; }
		QLineEdit *textField(const QString &name) const { return m_texts.value(name); }
		QComboBox *choiceField(const QString &name) const { return m_choices.value(name); }
		QLabel *rolesLabel() const { return m_roles; }
		QLabel *resultsLabel() const { return m_results; }
		QPushButton *catalogueButton() const { return m_catalogue; }
			/**
				Puts the data of @p entry in the fields of the device. The data
				the tab does not show (I2, instantaneous trip, I2t) go with it
				while the fields keep the values of the catalogue
			*/
		void applyDevice(const electrical::DeviceEntry &entry);

	protected:
		void enableLiveEdit() override;
		void disableLiveEdit() override;

	private:
		void buildGui();
		void fillProtection(const electrical::ProtectionData &protection);
		void updateState();
		QETProject *project() const;
		QLineEdit *addText(class QFormLayout *form, const QString &name,
						   const QString &label, const QString &unit = QString());
		QComboBox *addChoice(class QFormLayout *form, const QString &name,
							 const QString &label, const QList<QPair<QString, int>> &choices);

		QLabel *m_roles = nullptr;
		QComboBox *m_board = nullptr;
		QComboBox *m_circuit = nullptr;
		QGroupBox *m_protection_box = nullptr;
		QLabel *m_results = nullptr;
		QLabel *m_read_only = nullptr;
		QPushButton *m_catalogue = nullptr;
			/// The device chosen from the catalogue, not yet in the model
		std::optional<electrical::DeviceEntry> m_pending;
		QMap<QString, QLineEdit *> m_texts;
		QMap<QString, QComboBox *> m_choices;
		QList<QMetaObject::Connection> m_live;
		QMetaObject::Connection m_model_changed;
		bool m_filling = false;
};

#endif // ELECTRICALELEMENTWIDGET_H
