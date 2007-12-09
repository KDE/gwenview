// Qt
#include <QApplication>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

// Local
#include "../lib/slidecontainer.h"


using namespace Gwenview;


class Window : public QWidget {
public:
	Window()
	: QWidget() {
		SlideContainer* container = new SlideContainer(this);

		QPushButton* inButton = new QPushButton(this);
		inButton->setText("Slide &In");
		connect(inButton, SIGNAL(clicked()), container, SLOT(slideIn()));

		QPushButton* outButton = new QPushButton(this);
		outButton->setText("Slide &Out");
		connect(outButton, SIGNAL(clicked()), container, SLOT(slideOut()));

		QVBoxLayout* layout = new QVBoxLayout(this);
		layout->addWidget(inButton);
		layout->addWidget(outButton);
		layout->addWidget(container);

		QLineEdit* content = new QLineEdit(container);
		content->setText("Some long text. Some long text. Some long text. Some long text.");
		container->setContent(content);
	}
};


int main(int argc, char* argv[]) {
	QApplication app(argc, argv);
	Window window;

	window.show();
	return app.exec();
}
