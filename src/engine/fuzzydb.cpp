#include "fuzzydb.h"
#include "documentdb.h"
#include "postingdb.h"
#include "postingcodec.h"
#include <QtDebug>

#include <QTextBoundaryFinder>

using namespace Baloo;

class OwningPostingIterator : public PostingIterator {
public:
  OwningPostingIterator();
  quint64 docId() const override;
  quint64 next() override;

  void push(quint64 id);

private:
  QVector<quint64> m_vec;
  int m_pos;
};


FuzzyDB::FuzzyDB(MDB_dbi dbi, MDB_txn* txn)
  : m_txn(txn)
  , m_dbi(dbi)
{
  Q_ASSERT(txn != nullptr);
  Q_ASSERT(dbi != 0);
}

FuzzyDB::~FuzzyDB()
{
}

MDB_dbi FuzzyDB::create(MDB_txn* txn)
{
  MDB_dbi dbi;
  int rc = mdb_dbi_open(txn, "fuzzydb", MDB_CREATE, &dbi);
  Q_ASSERT_X(rc == 0, "FuzzyDB::create", mdb_strerror(rc));

  return dbi;
}

MDB_dbi FuzzyDB::open(MDB_txn* txn)
{
  MDB_dbi dbi;
  int rc = mdb_dbi_open(txn, "fuzzydb", 0, &dbi);
  if (rc == MDB_NOTFOUND) {
    qDebug() << "fuzzy open failed";
    return 0;
  }
  Q_ASSERT_X(rc == 0, "FuzzyDB::open", mdb_strerror(rc));

  return dbi;
}

void FuzzyDB::put(const QByteArray& bigram, const PostingList& list)
{
  Q_ASSERT(!bigram.isEmpty());
  Q_ASSERT(!list.isEmpty());

  MDB_val key;
  key.mv_size = bigram.size();
  key.mv_data = static_cast<void*>(const_cast<char*>(bigram.constData()));

  PostingCodec codec;
  QByteArray arr = codec.encode(list);

  MDB_val val;
  val.mv_size = arr.size();
  val.mv_data = static_cast<void*>(arr.data());

  int rc = mdb_put(m_txn, m_dbi, &key, &val, 0);
  Q_ASSERT_X(rc == 0, "FuzzyDB::put", mdb_strerror(rc));
}

QByteArray FuzzyDB::raw_get(const QByteArray& bigram)
{
  Q_ASSERT(!bigram.isEmpty());

  MDB_val key;
  key.mv_size = bigram.size();
  key.mv_data = static_cast<void*>(const_cast<char*>(bigram.constData()));

  MDB_val val;
  int rc = mdb_get(m_txn, m_dbi, &key, &val);
  if (rc == MDB_NOTFOUND) {
    return QByteArray();
  }
  Q_ASSERT_X(rc == 0, "FuzzyDB::get", mdb_strerror(rc));

  return QByteArray::fromRawData(static_cast<char*>(val.mv_data), val.mv_size);
}

PostingList FuzzyDB::get(const QByteArray& bigram)
{
  QByteArray arr = this->raw_get(bigram);
  PostingCodec codec;
  return codec.decode(arr);
}

void FuzzyDB::sync_terms(const DocumentDB& filename_terms)
{
  // get all words (inefficient just for the proof-of-concept)
  QMap<quint64, QVector<QByteArray>> terms = filename_terms.toTestMap();

  // create an inverted index for bigram -> list of ids
  for (auto i = terms.constBegin(); i != terms.constEnd(); ++i) {

    for (const QByteArray& term : i.value()) {
      QString sterm = QString::fromUtf8(term);
      if (sterm.startsWith('F')) continue;

      QStringList bigrams = this->into_bigrams(sterm);

      for (const QString& bigram : bigrams) {
        QByteArray bytes = bigram.toUtf8();
        PostingList ids = this->get(bytes);

        if (!ids.contains(i.key())) {
          ids.append(i.key());
          this->put(bytes, ids);
        }
      }
    }
  }
}

PostingIterator* FuzzyDB::iter(const QByteArray& term)
{
  qDebug() << "FuzzyDB::iter" << term;
  QMap<quint64, int> scores;
  QStringList bigrams = this->into_bigrams(term);

  // Score all possible results
  for (const QString& bigram : bigrams) {
    const QByteArray ids = this->raw_get(bigram.toUtf8());

    for (int i = 0; i < ids.size(); i += sizeof(quint64)) {
      quint64 id = *((quint64*)(ids.constData() + i));
      int bump = scores.value(id, 0) + 1;
      scores[id] = bump;
    }
  }

  OwningPostingIterator* found = new OwningPostingIterator();

  // Filter away bad matches
  for (auto i = scores.constBegin(); i != scores.constEnd(); ++i) {
    if (i.value() + 2 >= bigrams.size()) {
      found->push(i.key());
    }
  }

  return found;
}

QStringList FuzzyDB::into_bigrams(const QString& term)
{
  QStringList grams;
  grams.reserve(term.size() - 1);

  // Get all the individual characters into `list`
  // TODO: use QTextBoundaryFinder to get bigrams

  // Generate bigrams here
  for (int i = 1; i < term.size(); i += 1) {
    grams.append(term.mid(i - 1, 2));
  }
  return grams;
}

// OwningPostingIterator

OwningPostingIterator::OwningPostingIterator()
  : m_vec()
  , m_pos(0)
{
}

quint64 OwningPostingIterator::docId() const
{
  if (m_pos < 0 || m_pos >= m_vec.size()) {
    return 0;
  }

  return m_vec[m_pos];
}

quint64 OwningPostingIterator::next()
{
  if (m_pos >= m_vec.size() - 1) {
    m_pos = m_vec.size();
    return 0;
  }

  m_pos++;
  return m_vec[m_pos];
}

void OwningPostingIterator::push(quint64 id)
{
  this->m_vec.append(id);
}
