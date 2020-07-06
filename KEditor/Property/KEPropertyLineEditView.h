#pragma once
#include "KEditorConfig.h"
#include "KEPropertyBaseView.h"
#include "KEPropertyModel.h"
#include <QLineEdit>
#include <QValidator>
#include <QMimeData>
#include <QUrl>
#include <QDropEvent>
#include <assert.h>

class KEPropertyLineEdit : public QLineEdit
{
protected:
	bool m_AcceptDrop;
public:
	explicit KEPropertyLineEdit(QWidget *parent = Q_NULLPTR)
		: QLineEdit(parent)
	{}

	explicit KEPropertyLineEdit(const QString& name, QWidget *parent = Q_NULLPTR)
		: QLineEdit(name, parent)
	{}

	~KEPropertyLineEdit() {}

	inline void SetAcceptDrop(bool acceptDrop) { m_AcceptDrop = acceptDrop; }

	void dragEnterEvent(QDragEnterEvent *event) override
	{
		event->setDropAction(Qt::MoveAction);
		event->accept();
	}

	void dragMoveEvent(QDragMoveEvent *event) override
	{
		event->setDropAction(Qt::MoveAction);
		event->accept();
	}

	void dropEvent(QDropEvent *event) override
	{
		const QMimeData* mineData = event->mimeData();
		if (mineData)
		{
			QList<QUrl> urls = mineData->urls();
			if (urls.length() == 1)
			{
				const QUrl& url = urls[0];
				QString urlStr = url.toString();
				setText(urlStr);
				emit editingFinished();
			}
		}
	}
};

template<typename T, size_t DIMENSION = 1>
class KEPropertyLineEditView : public KEPropertyView<T, DIMENSION>
{
protected:
	QWidget* m_MainWidget;
	KEPropertyLineEdit *m_Widget[DIMENSION];
	QHBoxLayout *m_Layout;
	bool m_LazyUpdate;
	bool m_AcceptDrop;

	template<typename T2>
	void SetWidgetValidator(QLineEdit& widget, const T2& default)
	{

	}

	template<>
	void SetWidgetValidator(QLineEdit& widget, const float& default)
	{
		QDoubleValidator* validator = KNEW QDoubleValidator(&widget);
		widget.setValidator(validator);
	}

	template<>
	void SetWidgetValidator(QLineEdit& widget, const int& default)
	{
		QIntValidator* validator = KNEW QIntValidator(&widget);
		widget.setValidator(validator);
	}

	template<typename T2>
	void SetWidgetText(QLineEdit& widget, const T2& value)
	{
		static_assert(false, "please implement data form qstring");
	}

	template<>
	void SetWidgetText(QLineEdit& widget, const std::string& value)
	{
		widget.setText(value.c_str());
	}

	template<>
	void SetWidgetText(QLineEdit& widget, const float& value)
	{
		widget.setText(QString::number(value));
	}

	template<>
	void SetWidgetText(QLineEdit& widget, const int& value)
	{
		widget.setText(QString::number(value));
	}

	template<typename T2>
	T2 DataFromQString(const QString& text)
	{
		static_assert(false, "please implement data form qstring");
		return default;
	}

	template<>
	std::string DataFromQString(const QString& text)
	{
		return text.toStdString();
	}

	template<>
	float DataFromQString(const QString& text)
	{
		return text.toFloat();
	}

	template<>
	int DataFromQString(const QString& text)
	{
		return text.toInt();
	}

	void SetModelData(size_t index, const QString& text)
	{
		T data = DataFromQString<T>(text);
		UpdateModelElement(index, data);
	}

	void SetWidgetValue(size_t index, const T& value) override
	{
		assert(index < DIMENSION);
		if (m_Widget[index])
		{
			SetWidgetText(*m_Widget[index], value);
		}
	}
public:
	KEPropertyLineEditView(ModelPtrType model)
		: KEPropertyView(model),
		m_MainWidget(nullptr),
		m_Layout(nullptr),
		m_LazyUpdate(false),
		m_AcceptDrop(false)
	{
		ZERO_ARRAY_MEMORY(m_Widget);
	}

	~KEPropertyLineEditView()
	{
	}

	inline void SetLazyUpdate(bool lazyUpdate) { m_LazyUpdate = lazyUpdate; }
	inline void SetAcceptDrop(bool acceptDrop) { m_AcceptDrop = acceptDrop; }

	QWidget* AllocWidget() override
	{
		m_MainWidget = KNEW QWidget();

		for (size_t i = 0; i < DIMENSION; ++i)
		{
			m_Widget[i] = KNEW KEPropertyLineEdit(m_MainWidget);
			m_Widget[i]->SetAcceptDrop(m_AcceptDrop);
			SetWidgetValidator(*m_Widget[i], T());

			if (m_LazyUpdate)
			{
				QObject::connect(m_Widget[i], &KEPropertyLineEdit::editingFinished, [=, this]()
				{
					QString newText = m_Widget[i]->text();
					SetModelData(i, newText);
				});
			}
			else
			{
				QObject::connect(m_Widget[i], &KEPropertyLineEdit::textEdited, [=, this](const QString& newText)
				{
					SetModelData(i, newText);
				});
			}
		}

		m_Layout = KNEW QHBoxLayout(m_MainWidget);
		for (size_t i = 0; i < DIMENSION; ++i)
		{
			m_Layout->addWidget(m_Widget[i]);
		}

		m_MainWidget->setLayout(m_Layout);

		UpdateView(*m_Model);

		return m_MainWidget;
	}
};

namespace KEditor
{
	template<typename T, size_t DIMENTSION = 1, typename... Types>
	inline std::shared_ptr<KEPropertyBaseView> MakeLineEditViewByModel(Types&&... args)
	{
		return std::shared_ptr<KEPropertyBaseView>
			(KNEW KEPropertyLineEditView<T, DIMENTSION>(
				std::forward<Types>(args)...));
	}

	template<typename T, size_t DIMENTSION = 1, typename... Types>
	inline std::shared_ptr<KEPropertyBaseView> MakeLineEditView(Types&&... args)
	{
		return std::shared_ptr<KEPropertyBaseView>
			(KNEW KEPropertyLineEditView<T, DIMENTSION>(
				KEditor::MakePropertyModel<T, DIMENTSION>(std::forward<Types>(args)...)));
	}

	template<typename T, size_t DIMENTSION>
	inline std::shared_ptr<KEPropertyBaseView> MakeLineEditView(std::initializer_list<T>&& list)
	{
		return std::shared_ptr<KEPropertyBaseView>
			(KNEW KEPropertyLineEditView<T, DIMENTSION>(KEditor::MakePropertyModel<T, DIMENTSION>(
				std::forward<decltype(list)>(list))));
	}
}