#include <QDebug>
#include <QObject>
#include <QTest>
#include "fuzzysearch.h"

using namespace Baloo;

class FuzzySearchTest : public QObject
{
  Q_OBJECT
private Q_SLOTS:

  void testWordFeatures1();
  void testWordFeatures2();
  void testWordFeatures3();
  void testWordFeatures4();
  void testWordFeatures5();

  void testFeatures();

  void testSearch();
};

void FuzzySearchTest::testWordFeatures1()
{
  QString word("repetition");
  QList<FuzzyFeature> feats({"re", "ep", "pe", "et", "ti", "it", "ti", "io", "on"});
  QList<FuzzyFeature> computed = FuzzySearch::word_to_features(word);
  QCOMPARE(computed, feats);
}

void FuzzySearchTest::testWordFeatures2()
{
  QString word("odd");
  QList<FuzzyFeature> feats({"od", "dd"});
  QList<FuzzyFeature> computed = FuzzySearch::word_to_features(word);
  QCOMPARE(computed, feats);
}

void FuzzySearchTest::testWordFeatures3()
{
  QString word("hi");
  QList<FuzzyFeature> feats({"hi"});
  QList<FuzzyFeature> computed = FuzzySearch::word_to_features(word);
  QCOMPARE(computed, feats);
}

void FuzzySearchTest::testWordFeatures4()
{
  QString word("");
  QList<FuzzyFeature> computed = FuzzySearch::word_to_features(word);
  QList<FuzzyFeature> feats;
  QCOMPARE(computed, feats);
}

void FuzzySearchTest::testWordFeatures5()
{
  QString word("cAsE");
  QList<FuzzyFeature> feats({"ca", "as", "se"});
  QList<FuzzyFeature> computed = FuzzySearch::word_to_features(word);
  QCOMPARE(computed, feats);
}

void FuzzySearchTest::testFeatures()
{
  QStringList terms({"notes", "april8", "2018", "md"});
  QMap<FuzzyFeature, FuzzyDataList> exported = FuzzySearch::features(1010, terms);
  QMap<FuzzyFeature, FuzzyDataList> correct;

  auto make = [](quint8 wid, quint8 len) -> FuzzyDataList {
    FuzzyData fdata;
    fdata.id = 1010;
    fdata.wid = wid;
    fdata.len = len;
    QList<FuzzyData> ids({fdata});
    FuzzyDataList dlist;
    dlist.m_datalist = ids;
    return dlist;
  };

  correct.insert("no", make(0, 5));
  correct.insert("ot", make(0, 5));
  correct.insert("te", make(0, 5));
  correct.insert("es", make(0, 5));

  correct.insert("ap", make(1, 6));
  correct.insert("pr", make(1, 6));
  correct.insert("ri", make(1, 6));
  correct.insert("il", make(1, 6));
  correct.insert("l8", make(1, 6));

  correct.insert("20", make(2, 4));
  correct.insert("01", make(2, 4));
  correct.insert("18", make(2, 4));

  correct.insert("md", make(3, 2));

  QCOMPARE(exported, correct);
}

void FuzzySearchTest::testSearch()
{
  QStringList file1({"notes", "april8", "2018", "md"});
  QStringList file2({"notes", "wednesday", "04092018", "md"});
  QStringList file3({"thisissomepoorlydelimitedfile"});
  QStringList file4({"bathsroom", "png"});
  QStringList file5({"livingroom", "png"});
  QStringList file6({"washroom", "png"});
  QStringList file7({"sittingrooms", "png"});
  QStringList file8({"CS4540"});
  QStringList file9({"CS4641"});
  QStringList file10({"CS4476"});
  QStringList file11({"CS2200"});
  QStringList file12({"LMC2200"});

  QMap<FuzzyFeature, FuzzyDataList> db;
  db.unite(FuzzySearch::features(1, file1));
  db.unite(FuzzySearch::features(2, file2));
  db.unite(FuzzySearch::features(3, file3));
  db.unite(FuzzySearch::features(4, file4));
  db.unite(FuzzySearch::features(5, file5));
  db.unite(FuzzySearch::features(6, file6));
  db.unite(FuzzySearch::features(7, file7));
  db.unite(FuzzySearch::features(8, file8));
  db.unite(FuzzySearch::features(9, file9));
  db.unite(FuzzySearch::features(10, file10));
  db.unite(FuzzySearch::features(11, file11));
  db.unite(FuzzySearch::features(12, file12));

  FuzzyDataList list;
  FuzzyDataGetter getter = [&](const FuzzyFeature& feat) -> const FuzzyDataList& {
    list.m_datalist.clear();
    for (auto i = db.find(feat); i != db.end() && i.key() == feat; ++i) {
      list.m_datalist.append(i->m_datalist);
    }
    return list;
  };

  FuzzySearch fuzzy(2);
  QList<quint64> results;

  results = fuzzy.search(QString("wensday"), getter);
  QCOMPARE(results, QList<quint64>({ 2 }));

  results = fuzzy.search(QString("poorly"), getter);
  QCOMPARE(results, QList<quint64>({ 3 }));

  results = fuzzy.search(QString("4540"), getter);
  QCOMPARE(results, QList<quint64>({ 8, 2 }));

  results = fuzzy.search(QString("2200"), getter);
  QCOMPARE(results, QList<quint64>({ 11, 12, 1, 2 }));

  results = fuzzy.search(QString("room"), getter);
  QCOMPARE(results, QList<quint64>({ 6, 4, 5, 7, 3 }));

  results = fuzzy.search(QString("noots"), getter);
  QCOMPARE(results, QList<quint64>({ 1, 2 }));

  results = fuzzy.search(QString("delemeted"), getter);
  QCOMPARE(results, QList<quint64>({ 3 }));
}

QTEST_MAIN(FuzzySearchTest)

#include "fuzzysearchtest.moc"
