#include "fuzzysearch.h"

#include <algorithm>

using namespace Baloo;


FuzzySearch::FuzzySearch(int tolerance)
  : m_tolerance(tolerance)
{
}


QList<FuzzyFeature> FuzzySearch::word_to_features(const QString& term)
{
  QList<FuzzyFeature> grams;
  if (term.size() < 2) {
    return grams;
  }
  grams.reserve(term.size() - 1);

  // TODO: use QTextBoundaryFinder to get correct graphemes
  for (int i = 1; i < term.size(); i += 1) {
    grams.append(term.mid(i - 1, 2).toLower().toUtf8());
  }
  return grams;
}


QMap<FuzzyFeature, FuzzyDataList> FuzzySearch::features(quint64 id, QStringList terms)
{
  QMap<FuzzyFeature, FuzzyDataList> output;
  quint8 wid = 0;

  for (const QString& word : terms) {
    // Get the features for every word
    QList<QByteArray> features = FuzzySearch::word_to_features(word);
    // Create an inverted index of the features
    for (const QByteArray& feature : features) {
      // make the associated data
      FuzzyData data;
      data.id = id;
      data.wid = wid;
      data.len = word.size();

      // make this feature linked to the data it came from
      auto existing = output.find(feature);

      if (existing == output.end()) {
        // `feature` not used before, make a new one and insert
        FuzzyDataList list;
        list.m_datalist.append(data);
        output.insert(feature, list);
      }
      else {
        // found existing `feature`, add this id and word size to it
        existing->m_datalist.append(data);
      }
    }
    if (wid < 255) {
      wid += 1;
    }
  }
  return output;
}


QList<quint64>
FuzzySearch::search(const QString& query, const FuzzyDataGetter getter)
{
  QList<FuzzyFeature> features = this->word_to_features(query);
  QMap<FuzzyData, int> scores;

  // Calculate how close each term is to query, the higher score the better
  for (const FuzzyFeature& feature : features) {
    // Get all documents of each feature
    const FuzzyDataList& documents = getter(feature);
    for (const FuzzyData& doc : documents.m_datalist) {
      // Keep track of the score of each document and its length
      auto score = scores.find(doc);
      if (score == scores.end()) {
        scores.insert(doc, 1);
      }
      else {
        *score += 1;
      }
    }
  }

  // Remove based on threshold
  QList<QPair<FuzzyData, int>> passing;
  for (auto i = scores.begin(); i != scores.end(); ++i) {
    if (i.value() + this->m_tolerance >= features.size()) {
      passing.append(qMakePair(i.key(), i.value()));
    }
  }

  // Sort based on best score & shortest word
  std::sort(passing.begin(), passing.end(),
            [](const QPair<FuzzyData, int>& a, const QPair<FuzzyData, int>& b) {
              if (a.second == b.second) {
                return a.first.len < b.first.len;
              }
              return a.second > b.second;
            });

  // Shed metadata and return only IDs
  QList<quint64> ids;
  ids.reserve(passing.size());
  for (const QPair<FuzzyData, int>& a : passing) {
    ids.append(a.first.id);
  }

  return ids;
}


QByteArray FuzzyDataList::into_bytes()
{
  return QByteArray(); // TODO:
}


FuzzyData FuzzyDataList::from_bytes()
{
  return FuzzyData(); // TODO:
}

bool FuzzyDataList::operator==(const FuzzyDataList &other) const
{
  return other.m_datalist == m_datalist;
}

bool FuzzyData::operator==(const FuzzyData &other) const
{
  return id == other.id && wid == other.wid && len == other.len;
}

bool FuzzyData::operator<(const FuzzyData &other) const
{
  if (id == other.id) {
    return wid < other.wid;
  }
  return id < other.id;
}
