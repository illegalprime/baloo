#ifndef BALOO_FUZZYSEARCH_H
#define BALOO_FUZZYSEARCH_H

#include "engine_export.h"

#include <QString>
#include <QMap>
#include <QList>
#include <QByteArray>

#include <functional>

namespace Baloo {
  class BALOO_ENGINE_EXPORT FuzzyData
  {
  public:
    quint64 id;
    quint8 wid;
    quint8 len;

    bool operator==(const FuzzyData &other) const;
    bool operator<(const FuzzyData& other) const;
  };

  class BALOO_ENGINE_EXPORT FuzzyDataList
  {
  public:
    QList<FuzzyData> m_datalist;

    QByteArray into_bytes();
    static FuzzyData from_bytes();

    bool operator==(const FuzzyDataList &other) const;
  };

  using FuzzyFeature = QByteArray;

  using FuzzyDataGetter =
    std::function<const FuzzyDataList&(const FuzzyFeature&)>;

  class BALOO_ENGINE_EXPORT FuzzySearch
  {
  public:
    FuzzySearch(int tolerance);

    static QMap<FuzzyFeature, FuzzyDataList> features(quint64 id, QStringList terms);

    static QList<FuzzyFeature> word_to_features(const QString& word);

    QList<quint64> search(const QString& query, const FuzzyDataGetter getter);

  private:
    int m_tolerance;
  };
}

#endif // BALOO_FUZZYSEARCH_H
