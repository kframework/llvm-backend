package org.kframework.backend.llvm.matching

import org.kframework.parser.kore.SymbolOrAlias
import org.kframework.backend.llvm.matching.pattern._

sealed trait Heuristic {
  val needsMatrix: Boolean

  def scoreAs[T](p: AsP[T], f: Fringe, c: Clause, key: Option[Pattern[Option[Occurrence]]], isEmpty: Boolean): Double = ???
  def scoreList[T](p: ListP[T], f: Fringe, c: Clause, key: Option[Pattern[Option[Occurrence]]], isEmpty: Boolean): Double = ???
  def scoreLiteral[T](p: LiteralP[T], f: Fringe, c: Clause, key: Option[Pattern[Option[Occurrence]]], isEmpty: Boolean): Double = ???
  def scoreMap[T](p: MapP[T], f: Fringe, c: Clause, key: Option[Pattern[Option[Occurrence]]], isEmpty: Boolean): Double = ???
  def scoreOr[T](p: OrP[T], f: Fringe, c: Clause, key: Option[Pattern[Option[Occurrence]]], isEmpty: Boolean): Double = ???
  def scoreSet[T](p: SetP[T], f: Fringe, c: Clause, key: Option[Pattern[Option[Occurrence]]], isEmpty: Boolean): Double = ???
  def scoreSymbol[T](p: SymbolP[T], f: Fringe, c: Clause, key: Option[Pattern[Option[Occurrence]]], isEmpty: Boolean): Double = ???
  def scoreVariable[T](p: VariableP[T], f: Fringe, c: Clause, key: Option[Pattern[Option[Occurrence]]], isEmpty: Boolean): Double = ???
  def scoreWildcard[T](p: WildcardP[T], f: Fringe, c: Clause, key: Option[Pattern[Option[Occurrence]]], isEmpty: Boolean): Double = ???

  def computeScoreForKey(c: AbstractColumn, key: Option[Pattern[Option[Occurrence]]]): Double

  def getBest(cols: Seq[(Column, Int)], matrix: Matrix): Seq[(Column, Int)] = {
    var result: List[(Column, Int)] = Nil
    var best = cols(0)._1.score(new MatrixColumn(matrix, 0))
    for (col <- cols) {
      import Ordering.Implicits._
      val score = col._1.score(new MatrixColumn(matrix, cols.indexOf(col)))
      if (score > best) {
        best = score
        result = col :: Nil
      } else if (score == best) {
        result = col :: result
      }
    }
    result
  }

  def breakTies(cols: Seq[(Column, Int)]): (Column, Int) = RPseudoHeuristic.breakTies(cols)
}

object DefaultHeuristic extends Heuristic {
  val needsMatrix: Boolean = false

  override def scoreAs[T](p: AsP[T], f: Fringe, c: Clause, key: Option[Pattern[Option[Occurrence]]], isEmpty: Boolean): Double = p.pat.score(this, f, c, key, isEmpty)
  override def scoreList[T](p: ListP[T], f: Fringe, c: Clause, key: Option[Pattern[Option[Occurrence]]], isEmpty: Boolean): Double = 1.0
  override def scoreLiteral[T](p: LiteralP[T], f: Fringe, c: Clause, key: Option[Pattern[Option[Occurrence]]], isEmpty: Boolean): Double = 1.0
  override def scoreMap[T](p: MapP[T], f: Fringe, c: Clause, key: Option[Pattern[Option[Occurrence]]], isEmpty: Boolean): Double = {
    if (p.keys.isEmpty && p.frame.isEmpty) {
      1.0
    } else if (isEmpty) {
      0.0
    } else if (p.keys.isEmpty) {
      p.frame.get.score(this, f, c, key, isEmpty)
    } else if (key.isDefined) {
      if (p.canonicalize(c).keys.contains(key.get)) 1.0 else 0.0
    } else {
      1.0
    }
  }

  override def scoreOr[T](p: OrP[T], f: Fringe, c: Clause, key: Option[Pattern[Option[Occurrence]]], isEmpty: Boolean): Double = {
    p.ps.map(_.score(this, f, c, key, isEmpty)).sum
  }

  override def scoreSet[T](p: SetP[T], f: Fringe, c: Clause, key: Option[Pattern[Option[Occurrence]]], isEmpty: Boolean): Double = {
    if (p.elements.isEmpty && p.frame.isEmpty) { 
      1.0
    } else if (isEmpty) {
      0.0
    } else if (p.elements.isEmpty) {
      p.frame.get.score(this, f, c, key, isEmpty)
    } else if (key.isDefined) {
      if (p.canonicalize(c).elements.contains(key.get)) 1.0 else 0.0
    } else {
      1.0
    }
  }

  override def scoreSymbol[T](p: SymbolP[T], f: Fringe, c: Clause, key: Option[Pattern[Option[Occurrence]]], isEmpty: Boolean): Double = {
    val ncons = f.overloads(p.sym).size + 1.0
    1.0 / ncons
  }

  override def scoreVariable[T](p: VariableP[T], f: Fringe, c: Clause, key: Option[Pattern[Option[Occurrence]]], isEmpty: Boolean): Double = 0.0
  override def scoreWildcard[T](p: WildcardP[T], f: Fringe, c: Clause, key: Option[Pattern[Option[Occurrence]]], isEmpty: Boolean): Double = 0.0

  def computeScoreForKey(c: AbstractColumn, key: Option[Pattern[Option[Occurrence]]]): Double = {
    var result = 0.0
    for (i <- c.column.patterns.indices) {
      if (c.column.clauses(i).action.priority != c.column.clauses.head.action.priority)
        return result
      result += c.column.patterns(i).score(DefaultHeuristic, c.column.fringe, c.column.clauses(i), key, c.column.isEmpty)
    }
    result
  }
}

object FHeuristic extends Heuristic {
  val needsMatrix: Boolean = false

  def computeScoreForKey(c: AbstractColumn, key: Option[Pattern[Option[Occurrence]]]): Double = {
    for (i <- c.column.patterns.indices) {
      if (c.column.clauses(i).action.priority != c.column.clauses.head.action.priority)
        return 1.0
      if (c.column.patterns(i).isWildcard) {
        return 0.0
      }
    }
    1.0
  }
}

object DHeuristic extends Heuristic {
  val needsMatrix: Boolean = false

  def computeScoreForKey(c: AbstractColumn, key: Option[Pattern[Option[Occurrence]]]): Double = {
    -(c.column.patterns.count(_.isDefault))
  }
}

object BHeuristic extends Heuristic {
  val needsMatrix: Boolean = false

  def computeScoreForKey(c: AbstractColumn, key: Option[Pattern[Option[Occurrence]]]): Double = {
    val sigma = c.column.signatureForKey(key)
    if (c.column.category.hasIncompleteSignature(sigma, c.column.fringe)) {
      -sigma.size-1
    } else {
      -sigma.size
    }
  }
}

object AHeuristic extends Heuristic {
  val needsMatrix: Boolean = false

  def computeScoreForKey(c: AbstractColumn, key: Option[Pattern[Option[Occurrence]]]): Double = {
    var result = 0.0
    for (con <- c.column.signatureForKey(key)) {
      result -= c.column.fringe.expand(con).size
    }
    result
  }
}

object RHeuristic extends Heuristic {
  val needsMatrix: Boolean = false

  def computeScoreForKey(c: AbstractColumn, key: Option[Pattern[Option[Occurrence]]]): Double = {
    var result = 0.0
    val signature = c.column.signatureForKey(key)
    for (con <- signature) {
      for (i <- c.column.patterns.indices) {
        if (c.column.patterns(i).isSpecialized(con, c.column.fringe, c.column.clauses(i), c.column.maxPriority)) {
          result += 1.0
        }
      }
    }

    if (c.column.category.hasIncompleteSignature(signature, c.column.fringe)) {
      for (i <- c.column.patterns.indices) {
        if (c.column.patterns(i).isDefault) {
          result += 1.0
        }
      }
    }

    -result
  }
}

object QHeuristic extends Heuristic {
  val needsMatrix: Boolean = false

  def computeScoreForKey(c: AbstractColumn, key: Option[Pattern[Option[Occurrence]]]): Double = {
    var result = 0
    var priority = c.column.clauses.head.action.priority
    for (i <- c.column.patterns.indices) {
      if (c.column.clauses(i).action.priority != priority) {
        if (result != i) {
          return result
        }
        priority = c.column.clauses(i).action.priority
      }
      if (!c.column.patterns(i).isWildcard) {
        result += 1
      }
    }
    result
  }
}

sealed trait PseudoHeuristic extends Heuristic {
  val needsMatrix: Boolean = false

  def computeScoreForKey(c: AbstractColumn, key: Option[Pattern[Option[Occurrence]]]): Double = 0.0
}

object NPseudoHeuristic extends PseudoHeuristic {
  override def breakTies(cols: Seq[(Column, Int)]): (Column, Int) = {
    cols(0)
  }
}

object LPseudoHeuristic extends PseudoHeuristic {
  override def breakTies(cols: Seq[(Column, Int)]): (Column, Int) = {
    cols.minBy(_._1.fringe.occurrence.size)
  }
}
object RPseudoHeuristic extends PseudoHeuristic {
  override def breakTies(cols: Seq[(Column, Int)]): (Column, Int) = {
    cols.reverse.minBy(_._1.fringe.occurrence.size)
  }
}
