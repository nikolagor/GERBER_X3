/*******************************************************************************
*                                                                              *
* Author    :  Angus Johnson                                                   *
* Version   :  6.4.2                                                           *
* Date      :  27 February 2017                                                *
* Website   :  http://www.angusj.com                                           *
* Copyright :  Angus Johnson 2010-2017                                         *
*                                                                              *
* License:                                                                     *
* Use, modification & distribution is subject to Boost Software License Ver 1. *
* http://www.boost.org/LICENSE_1_0.txt                                         *
*                                                                              *
* Attributions:                                                                *
* The code in this library is an extension of Bala Vatti's clipping algorithm: *
* "A generic solution to polygon clipping"                                     *
* Communications of the ACM, Vol 35, Issue 7 (July 1992) pp 56-63.             *
* http://portal.acm.org/citation.cfm?id=129906                                 *
*                                                                              *
* Computer graphics and geometric modeling: implementation and algorithms      *
* By Max K. Agoston                                                            *
* Springer; 1 edition (January 4, 2005)                                        *
* http://books.google.com/books?q=vatti+clipping+agoston                       *
*                                                                              *
* See also:                                                                    *
* "Polygon Offsetting by Computing Winding Numbers"                            *
* Paper no. DETC2005-85513 pp. 565-575                                         *
* ASME 2005 International Design Engineering Technical Conferences             *
* and Computers and Information in Engineering Conference (IDETC/CIE2005)      *
* September 24-28, 2005 , Long Beach, California, USA                          *
* http://www.me.berkeley.edu/~mcmains/pubs/DAC05OffsetPolygon.pdf              *
*                                                                              *
*******************************************************************************/

#ifndef clipper_hpp
#define clipper_hpp

#define CLIPPER_VERSION "6.4.2"

//use_int32: When enabled 32bit ints are used instead of 64bit ints. This
//improve performance but coordinate values are limited to the range +/- 46340
//#define use_int32

//use_xyz: adds a Z member to IntPoint. Adds a minor cost to perfomance.
//#define use_xyz

//use_lines: Enables line clipping. Adds a very minor cost to performance.
#define use_lines

//use_deprecated: Enables temporary support for the obsolete functions
//#define use_deprecated

#include "mvector.h"

#include <cstdlib>
#include <cstring>
#include <functional>
#include <list>
#include <ostream>
#include <queue>
#include <set>
#include <stdexcept>
#include <stdint.h>
#include <tuple>

#include <QDataStream>
#include <QDebug>
#include <QPolygonF>
#include <QtMath>

#include <numbers>
using std::numbers::pi;
constexpr auto two_pi = pi * 2;

namespace ClipperLib {

enum ClipType {
    ctIntersection,
    ctUnion,
    ctDifference,
    ctXor
};

enum PolyType {
    ptSubject,
    ptClip
};

//By far the most widely used winding rules for polygon filling are
//EvenOdd & NonZero (GDI, GDI+, XLib, OpenGL, Cairo, AGG, Quartz, SVG, Gr32)
//Others rules include Positive, Negative and ABS_GTR_EQ_TWO (only in OpenGL)
//see http://glprogramming.com/red/chapter11.html
enum PolyFillType {
    pftEvenOdd,
    pftNonZero,
    pftPositive,
    pftNegative
};

#ifdef use_int32
typedef int cInt;
static cInt const loRange = 0x7FFF;
static cInt const hiRange = 0x7FFF;
#else
typedef int32_t cInt;
static cInt constexpr loRange = std::numeric_limits<int32_t>::max();
static cInt constexpr hiRange = std::numeric_limits<int32_t>::max(); // 0x40000000; //FFFFFFFFL /*L*/;
typedef signed long long long64;                                     //used by Int128 class
typedef unsigned long long ulong64;

constexpr cInt uScale = 100000;
constexpr double dScale = 1.0 / uScale;
#endif

struct IntPoint {
    cInt X;
    cInt Y;
#ifdef use_xyz
    cInt Z;
    IntPoint(cInt x = 0, cInt y = 0, cInt z = 0) F
        : X(x),
          Y(y),
          Z(z) {};
#else
#endif
    IntPoint(cInt x = 0, cInt y = 0) noexcept
        : X(x)
        , Y(y) {
    }

    IntPoint(IntPoint&& p) noexcept = default;
    IntPoint(const IntPoint& p) noexcept = default;
    IntPoint& operator=(IntPoint&& p) noexcept = default;
    IntPoint& operator=(const IntPoint& p) noexcept = default;
    IntPoint(QPointF&& p) noexcept
        : X(p.x() * uScale)
        , Y(p.y() * uScale) {
    }
    IntPoint(const QPointF& p) noexcept
        : X(p.x() * uScale)
        , Y(p.y() * uScale) {
    }
    IntPoint& operator=(QPointF&& p) noexcept {
        X = p.x() * uScale;
        Y = p.y() * uScale;
        return *this;
    }
    IntPoint& operator=(const QPointF& p) noexcept {
        X = p.x() * uScale;
        Y = p.y() * uScale;
        return *this;
    }

    bool isNull() const noexcept {
        return X == 0 && Y == 0;
    }

    IntPoint& operator*=(double s) noexcept {
        return X *= s, Y *= s, *this;
    }

    IntPoint& operator+=(const IntPoint& pt) noexcept {
        return X += pt.X, Y += pt.Y, *this;
    }

    IntPoint& operator-=(const IntPoint& pt) noexcept {
        return X -= pt.X, Y -= pt.Y, *this;
    }

    operator QPointF() const noexcept {
        return { X * dScale, Y * dScale };
    }

#ifdef __GNUC___
    bool operator==(const IntPoint& L) const noexcept {
        return X == L.X && Y == L.Y;
    }
    bool operator!=(const IntPoint& L) const noexcept {
        return !(*this == L);
    }
    bool operator<(const IntPoint& L) const noexcept {
        return std::tuple { X, Y } < std::tuple { L.X, L.Y };
    }
#else
    auto operator<=>(const IntPoint&) const noexcept = default;
#endif

    friend QDataStream& operator<<(QDataStream& stream, const IntPoint& pt) {
        stream.writeRawData(reinterpret_cast<const char*>(&pt), sizeof(IntPoint));
        return stream;
    }

    friend QDataStream& operator>>(QDataStream& stream, IntPoint& pt) {
        stream.readRawData(reinterpret_cast<char*>(&pt), sizeof(IntPoint));
        return stream;
    }

    friend QDebug operator<<(QDebug d, const IntPoint& p) {
        d << "IntPt(" << p.X << ", " << p.Y << ")";
        return d;
    }

    double angleTo(const IntPoint& pt2) const noexcept {
        const double dx = pt2.X - X;
        const double dy = pt2.Y - Y;
        const double theta = atan2(-dy, dx) * 360.0 / (M_PI * 2);
        const double theta_normalized = theta < 0 ? theta + 360 : theta;
        if (qFuzzyCompare(theta_normalized, double(360)))
            return 0.0;
        else
            return theta_normalized;
    }

    double angleRadTo(const IntPoint& pt2) const noexcept {
        const double dx = pt2.X - X;
        const double dy = pt2.Y - Y;
        const double theta = atan2(-dy, dx);
        return theta;
        const double theta_normalized = theta < 0 ? theta + (M_PI * 2) : theta;
        if (qFuzzyCompare(theta_normalized, (M_PI * 2)))
            return 0.0;
        else
            return theta_normalized;
    }

    double distTo(const IntPoint& pt2) const noexcept {
        double x = pt2.X - X;
        double y = pt2.Y - Y;
        return sqrt(x * x + y * y);
    }
    double distToSq(const IntPoint& pt2) const noexcept {
        double x = pt2.X - X;
        double y = pt2.Y - Y;
        return (x * x + y * y);
    }
};

//------------------------------------------------------------------------------
struct Path : mvector<IntPoint> {
    using MV = mvector<IntPoint>;
    using MV::MV;

    Path(const QPolygonF& v) {
        MV::reserve(v.size());
        for (auto& pt : v) {
            MV::push_back(pt);
        }
    }

    operator QPolygonF() const {
        QPolygonF poly;
        poly.reserve(int(size() + 1)); // +1 if need closed polygons
        for (const auto pt : *this)
            poly << pt;
        return poly;
    }
    //    int id {};
};

struct Paths : mvector<Path> {
    using MV = mvector<Path>;
    using MV::MV;

    Paths(const MV& v)
        : MV(v) { }
    Paths(MV&& v)
        : MV(std::move(v)) { }

    Paths(const QList<QPolygonF>& v) {
        MV::reserve(v.size());
        for (auto&& qpolygonf : v)
            MV::emplace_back(qpolygonf);
    }

    operator mvector<QPolygonF>() const {
        mvector<QPolygonF> polys;
        polys.reserve(size());
        for (const auto& poly : *this)
            polys.emplace_back(poly);
        return polys;
    }

    //    int id {};
};

//inline Path& operator<<(Path& poly, const IntPoint& p)
//{
//    poly.push_back(p);
//    return poly;
//}
//inline Paths& operator<<(Paths& polys, const Path& p)
//{
//    polys.push_back(p);
//    return polys;
//}

std::ostream& operator<<(std::ostream& s, const IntPoint& p);
std::ostream& operator<<(std::ostream& s, const Path& p);
std::ostream& operator<<(std::ostream& s, const Paths& p);

struct DoublePoint {
    double X;
    double Y;
    DoublePoint(double x = 0, double y = 0)
        : X(x)
        , Y(y) {
    }
    DoublePoint(IntPoint ip)
        : X(static_cast<double>(ip.X))
        , Y(static_cast<double>(ip.Y)) {
    }
};
//------------------------------------------------------------------------------

#ifdef use_xyz
typedef void (*ZFillCallback)(IntPoint& e1bot, IntPoint& e1top, IntPoint& e2bot, IntPoint& e2top, IntPoint& pt);
#endif

enum InitOptions { ioReverseSolution = 1,
    ioStrictlySimple = 2,
    ioPreserveCollinear = 4 };
enum JoinType { jtSquare,
    jtRound,
    jtMiter };
enum EndType { etClosedPolygon,
    etClosedLine,
    etOpenButt,
    etOpenSquare,
    etOpenRound };

class PolyNode;
typedef mvector<PolyNode*> PolyNodes;

class PolyNode {
public:
    PolyNode();
    /*virtual*/ ~PolyNode() { }
    Path Contour;
    PolyNodes Childs;
    PolyNode* Parent;
    PolyNode* GetNext() const;
    bool IsHole() const;
    bool IsOpen() const;
    size_t ChildCount() const;
    int Nesting = 0;

private:
    //PolyNode& operator =(PolyNode& other);
    size_t Index; //node index in Parent.Childs
    bool m_IsOpen;
    JoinType m_jointype;
    EndType m_endtype;
    PolyNode* GetNextSiblingUp() const;
    void AddChild(PolyNode& child);
    friend class Clipper; //to access Index
    friend class ClipperOffset;
};

class PolyTree : public PolyNode {
public:
    ~PolyTree() { Clear(); }
    PolyNode* GetFirst() const;
    void Clear();
    int Total() const;

private:
    //PolyTree& operator =(PolyTree& other);
    PolyNodes AllNodes;
    friend class Clipper; //to access AllNodes
};

bool Orientation(const Path& poly);
double Area(const Paths& polygons);
double Area(const Path& poly);
int PointInPolygon(const IntPoint& pt, const Path& path);

void SimplifyPolygon(const Path& in_poly, Paths& out_polys, PolyFillType fillType = pftEvenOdd);
void SimplifyPolygons(const Paths& in_polys, Paths& out_polys, PolyFillType fillType = pftEvenOdd);
void SimplifyPolygons(Paths& polys, PolyFillType fillType = pftEvenOdd);

void CleanPolygon(const Path& in_poly, Path& out_poly, double distance = 1.415);
void CleanPolygon(Path& poly, double distance = 1.415);
void CleanPolygons(const Paths& in_polys, Paths& out_polys, double distance = 1.415);
void CleanPolygons(Paths& polys, double distance = 1.415);

void MinkowskiSum(const Path& pattern, const Path& path, Paths& solution, bool pathIsClosed);
void MinkowskiSum(const Path& pattern, const Paths& paths, Paths& solution, bool pathIsClosed);
void MinkowskiDiff(const Path& poly1, const Path& poly2, Paths& solution);

void PolyTreeToPaths(const PolyTree& polytree, Paths& paths);
void ClosedPathsFromPolyTree(const PolyTree& polytree, Paths& paths);
void OpenPathsFromPolyTree(const PolyTree& polytree, Paths& paths);

Path& ReversePath(Path& p);
Paths& ReversePaths(Paths& p);

struct IntRect {
    cInt left;
    cInt top;
    cInt right;
    cInt bottom;
    cInt height() const { return bottom - top; }
    cInt width() const { return right - left; }
};

enum EdgeSide {
    esLeft = 1,
    esRight = 2
};

//forward declarations (for stuff used internally) ...
struct TEdge;
struct IntersectNode;
struct LocalMinimum;
struct OutPt;
struct OutRec;
struct Join;

typedef mvector /*mvector*/<OutRec*> PolyOutList;
typedef mvector /*mvector*/<TEdge*> EdgeList;
typedef mvector /*mvector*/<Join*> JoinList;
typedef mvector /*mvector*/<IntersectNode*> IntersectList;

//------------------------------------------------------------------------------

//ClipperBase is the ancestor to the Clipper class. It should not be
//instantiated directly. This class simply abstracts the conversion of sets of
//polygon coordinates into edge objects that are stored in a LocalMinima list.
class ClipperBase {
public:
    ClipperBase();
    virtual ~ClipperBase();
    virtual bool AddPath(const Path& pg, PolyType PolyTyp, bool Closed = true);
    bool AddPaths(const Paths& ppg, PolyType PolyTyp, bool Closed = true);
    virtual void Clear();
    IntRect GetBounds();
    bool PreserveCollinear() { return m_PreserveCollinear; }
    void PreserveCollinear(bool value) { m_PreserveCollinear = value; }

protected:
    void DisposeLocalMinimaList();
    TEdge* AddBoundsToLML(TEdge* e, bool IsClosed);
    virtual void Reset();
    TEdge* ProcessBound(TEdge* E, bool IsClockwise);
    void InsertScanbeam(const cInt Y);
    bool PopScanbeam(cInt& Y);
    bool LocalMinimaPending();
    bool PopLocalMinima(cInt Y, const LocalMinimum*& locMin);
    OutRec* CreateOutRec();
    void DisposeAllOutRecs();
    void DisposeOutRec(PolyOutList::size_type index);
    void SwapPositionsInAEL(TEdge* edge1, TEdge* edge2);
    void DeleteFromAEL(TEdge* e);
    void UpdateEdgeIntoAEL(TEdge*& e);

    typedef mvector /*mvector*/<LocalMinimum> MinimaList;
    MinimaList::iterator m_CurrentLM;
    MinimaList m_MinimaList;

    bool m_UseFullRange;
    EdgeList m_edges;
    bool m_PreserveCollinear;
    bool m_HasOpenPaths;
    PolyOutList m_PolyOuts;
    TEdge* m_ActiveEdges;

    typedef std::priority_queue<cInt> ScanbeamList;
    ScanbeamList m_Scanbeam;
};

class cancelException : public std::exception {
public:
    cancelException(const char* description)
        : m_descr(description) {
    }
    ~cancelException() noexcept override = default;
    const char* what() const noexcept override { return m_descr.c_str(); }

private:
    std::string m_descr;
};

//------------------------------------------------------------------------------

class Clipper : public virtual ClipperBase {
public:
    Clipper(int initOptions = 0);
    bool Execute(ClipType clipType,
        Paths& solution,
        PolyFillType fillType = pftEvenOdd);
    bool Execute(ClipType clipType,
        Paths& solution,
        PolyFillType subjFillType,
        PolyFillType clipFillType);
    bool Execute(ClipType clipType,
        PolyTree& polytree,
        PolyFillType fillType = pftEvenOdd);
    bool Execute(ClipType clipType,
        PolyTree& polytree,
        PolyFillType subjFillType,
        PolyFillType clipFillType);
    bool ReverseSolution() { return m_ReverseOutput; }
    void ReverseSolution(bool value) { m_ReverseOutput = value; }
    bool StrictlySimple() { return m_StrictSimple; }
    void StrictlySimple(bool value) { m_StrictSimple = value; }
//set the callback function for z value filling on intersections (otherwise Z is 0)
#ifdef use_xyz
    void ZFillFunction(ZFillCallback zFillFunc);
#endif
protected:
    /*virtual*/ bool ExecuteInternal();

private:
    JoinList m_Joins;
    JoinList m_GhostJoins;
    IntersectList m_IntersectList;
    ClipType m_ClipType;
    typedef std::list<cInt> MaximaList;
    MaximaList m_Maxima;
    TEdge* m_SortedEdges;
    bool m_ExecuteLocked;
    PolyFillType m_ClipFillType;
    PolyFillType m_SubjFillType;
    bool m_ReverseOutput;
    bool m_UsingPolyTree;
    bool m_StrictSimple;
#ifdef use_xyz
    ZFillCallback m_ZFill; //custom callback
#endif
    void SetWindingCount(TEdge& edge);
    bool IsEvenOddFillType(const TEdge& edge) const;
    bool IsEvenOddAltFillType(const TEdge& edge) const;
    void InsertLocalMinimaIntoAEL(const cInt botY);
    void InsertEdgeIntoAEL(TEdge* edge, TEdge* startEdge);
    void AddEdgeToSEL(TEdge* edge);
    bool PopEdgeFromSEL(TEdge*& edge);
    void CopyAELToSEL();
    void DeleteFromSEL(TEdge* e);
    void SwapPositionsInSEL(TEdge* edge1, TEdge* edge2);
    bool IsContributing(const TEdge& edge) const;
    bool IsTopHorz(const cInt XPos);
    void DoMaxima(TEdge* e);
    void ProcessHorizontals();
    void ProcessHorizontal(TEdge* horzEdge);
    void AddLocalMaxPoly(TEdge* e1, TEdge* e2, const IntPoint& pt);
    OutPt* AddLocalMinPoly(TEdge* e1, TEdge* e2, const IntPoint& pt);
    OutRec* GetOutRec(int idx);
    void AppendPolygon(TEdge* e1, TEdge* e2);
    void IntersectEdges(TEdge* e1, TEdge* e2, IntPoint& pt);
    OutPt* AddOutPt(TEdge* e, const IntPoint& pt);
    OutPt* GetLastOutPt(TEdge* e);
    bool ProcessIntersections(const cInt topY);
    void BuildIntersectList(const cInt topY);
    void ProcessIntersectList();
    void ProcessEdgesAtTopOfScanbeam(const cInt topY);
    void BuildResult(Paths& polys);
    void BuildResult2(PolyTree& polytree);
    void SetHoleState(TEdge* e, OutRec* outrec);
    void DisposeIntersectNodes();
    bool FixupIntersectionOrder();
    void FixupOutPolygon(OutRec& outrec);
    void FixupOutPolyline(OutRec& outrec);
    bool IsHole(TEdge* e);
    bool FindOwnerFromSplitRecs(OutRec& outRec, OutRec*& currOrfl);
    void FixHoleLinkage(OutRec& outrec);
    void AddJoin(OutPt* op1, OutPt* op2, const IntPoint offPt);
    void ClearJoins();
    void ClearGhostJoins();
    void AddGhostJoin(OutPt* op, const IntPoint offPt);
    bool JoinPoints(Join* j, OutRec* outRec1, OutRec* outRec2);
    void JoinCommonEdges();
    void DoSimplePolygons();
    void FixupFirstLefts1(OutRec* OldOutRec, OutRec* NewOutRec);
    void FixupFirstLefts2(OutRec* InnerOutRec, OutRec* OuterOutRec);
    void FixupFirstLefts3(OutRec* OldOutRec, OutRec* NewOutRec);
#ifdef use_xyz
    void SetZ(IntPoint& pt, TEdge& e1, TEdge& e2);
#endif
};
//------------------------------------------------------------------------------

class ClipperOffset {
public:
    ClipperOffset(double miterLimit = 2.0, double roundPrecision = 0.25);
    ~ClipperOffset();
    void AddPath(const Path& path, JoinType joinType, EndType endType);
    void AddPaths(const Paths& paths, JoinType joinType, EndType endType);
    void Execute(Paths& solution, double delta);
    void Execute(PolyTree& solution, double delta);
    void Clear();
    double MiterLimit;
    double ArcTolerance;

private:
    Paths m_destPolys;
    Path m_srcPoly;
    Path m_destPoly;
    mvector /*mvector*/<DoublePoint> m_normals;
    double m_delta, m_sinA, m_sin, m_cos;
    double m_miterLim, m_StepsPerRad;
    IntPoint m_lowest;
    PolyNode m_polyNodes;

    void FixOrientations();
    void DoOffset(double delta);
    void OffsetPoint(int j, int& k, JoinType jointype);
    void DoSquare(int j, int k);
    void DoMiter(int j, int k, double r);
    void DoRound(int j, int k);
};
//------------------------------------------------------------------------------

class clipperException : public std::exception {
public:
    clipperException(const char* description)
        : m_descr(description) {
    }
    ~clipperException() noexcept override = default;
    const char* what() const noexcept override { return m_descr.c_str(); }

private:
    std::string m_descr;
};
//------------------------------------------------------------------------------

} // namespace ClipperLib

#endif //clipper_hpp
