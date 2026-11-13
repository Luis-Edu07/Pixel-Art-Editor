#include "GifFormat.h"

#include <QFile>
#include <QHash>

namespace {
void writeWord(QIODevice &out, quint16 value) { char b[2]={char(value&255),char(value>>8)}; out.write(b,2); }
void writeSubBlocks(QIODevice &out,const QByteArray &data)
{
    for(int offset=0;offset<data.size();){const int count=qMin(255,data.size()-offset);out.putChar(char(count));out.write(data.constData()+offset,count);offset+=count;}
    out.putChar(0);
}
QByteArray lzw(const QByteArray &pixels)
{
    constexpr int clear=256,end=257; int codeSize=9,nextCode=258;
    QHash<quint32,int> dictionary;
    QByteArray packed; quint32 bits=0; int bitCount=0;
    auto emitCode=[&](int code){bits|=quint32(code)<<bitCount;bitCount+=codeSize;while(bitCount>=8){packed.append(char(bits&255));bits>>=8;bitCount-=8;}};
    emitCode(clear);
    if(pixels.isEmpty()){emitCode(end);if(bitCount)packed.append(char(bits&255));return packed;}
    int prefix=uchar(pixels[0]);
    for(int i=1;i<pixels.size();++i){const int symbol=uchar(pixels[i]);const quint32 key=(quint32(prefix)<<8)|symbol;
        if(dictionary.contains(key)){prefix=dictionary.value(key);continue;}
        emitCode(prefix);
        if(nextCode<4096){dictionary.insert(key,nextCode++);if(nextCode==(1<<codeSize)&&codeSize<12)++codeSize;}
        else{emitCode(clear);dictionary.clear();codeSize=9;nextCode=258;}
        prefix=symbol;
    }
    emitCode(prefix);emitCode(end);if(bitCount)packed.append(char(bits&255));return packed;
}
QByteArray indicesFor(const QImage &image)
{
    QByteArray result;result.resize(image.width()*image.height());
    for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x){const QColor c=image.pixelColor(x,y);int index=0;
        if(c.alpha()>=128){const int r=qBound(0,qRound(c.red()*5.0/255),5),g=qBound(0,qRound(c.green()*5.0/255),5),b=qBound(0,qRound(c.blue()*5.0/255),5);index=1+r*36+g*6+b;}
        result[y*image.width()+x]=char(index);
    }
    return result;
}
}

namespace GifFormat {
bool save(const QVector<QImage> &frames,int fps,bool loop,const QString &path,QString *error)
{
    if(frames.isEmpty()||frames.size()>512){if(error)*error=QObject::tr("A animação precisa ter de 1 a 512 quadros.");return false;}
    const QSize size=frames.first().size();if(size.width()<1||size.height()<1||size.width()>65535||size.height()>65535){if(error)*error=QObject::tr("Dimensões de GIF inválidas.");return false;}
    for(const QImage &frame:frames)if(frame.size()!=size){if(error)*error=QObject::tr("Todos os quadros precisam ter as mesmas dimensões.");return false;}
    QFile file(path);if(!file.open(QIODevice::WriteOnly)){if(error)*error=file.errorString();return false;}
    file.write("GIF89a",6);writeWord(file,quint16(size.width()));writeWord(file,quint16(size.height()));
    file.putChar(char(0xF7));file.putChar(0);file.putChar(0);
    for(int i=0;i<256;++i){if(i==0||i>216){file.putChar(0);file.putChar(0);file.putChar(0);}else{const int n=i-1;file.putChar(char((n/36)*51));file.putChar(char(((n/6)%6)*51));file.putChar(char((n%6)*51));}}
    if(loop){file.write("!\xFF\x0BNETSCAPE2.0\x03\x01",16);writeWord(file,0);file.putChar(0);}
    const quint16 delay=quint16(qBound(1,100/qMax(1,fps),65535));
    for(const QImage &frame:frames){file.write("!\xF9\x04\x05",4);writeWord(file,delay);file.putChar(0);file.putChar(0);
        file.putChar(',');writeWord(file,0);writeWord(file,0);writeWord(file,quint16(size.width()));writeWord(file,quint16(size.height()));file.putChar(0);
        file.putChar(8);writeSubBlocks(file,lzw(indicesFor(frame.convertToFormat(QImage::Format_ARGB32))));}
    file.putChar(';');
    if(file.error()!=QFileDevice::NoError){if(error)*error=file.errorString();return false;}
    return true;
}
}
