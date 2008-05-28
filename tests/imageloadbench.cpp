#include <QBuffer>
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QImage>
#include <QImageReader>
#include <QTime>

#include <lib/imageformats/imageformats.h>

const int ITERATIONS = 2;
const QSize SCALED_SIZE(1280, 800);


static void bench(QIODevice* device, const QString& outputName) {
	QTime chrono;
	chrono.start();
	for (int iteration=0; iteration<ITERATIONS; ++iteration) {
		qDebug() << "Iteration:" << iteration;

		device->open(QIODevice::ReadOnly);
		QImageReader reader(device);
		QSize size = reader.size();
		size.scale(SCALED_SIZE, Qt::KeepAspectRatio);
		reader.setScaledSize(size);
		QImage img = reader.read();
		device->close();

		if (iteration == ITERATIONS - 1) {
			qDebug() << "time:" << chrono.elapsed();
			img.save(outputName, "png");
		}
	}
}


int main(int argc, char** argv) {
	QCoreApplication app(argc, argv);

	QString fileName = QString::fromUtf8(argv[1]);

	QFile file(fileName);
	Q_ASSERT(file.open(QIODevice::ReadOnly));
	QByteArray data = file.readAll();
	QBuffer buffer(&data);

	bench(&buffer, "qt.png");
	Gwenview::ImageFormats::registerPlugins();
	bench(&buffer, "gv.png");

	return 0;
}
