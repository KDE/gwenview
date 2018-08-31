#include <QBuffer>
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QImage>
#include <QImageReader>
#include <QTime>

const int ITERATIONS = 2;
const QSize SCALED_SIZE(1280, 800);

static void bench(QIODevice* device, const QString& outputName)
{
    QTime chrono;
    chrono.start();
    for (int iteration = 0; iteration < ITERATIONS; ++iteration) {
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

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    if (argc != 2) {
        qDebug() << "Usage: imageloadbench <file.jpg>";
        return 1;
    }

    QString fileName = QString::fromUtf8(argv[1]);

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << QStringLiteral("Could not open '%1'").arg(fileName);
        return 2;
    }
    QByteArray data = file.readAll();
    QBuffer buffer(&data);

    qDebug() << "Using Qt loader";
    bench(&buffer, "qt.png");

    return 0;
}
