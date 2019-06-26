package org.kframework.backend.llvm.matching

import org.kframework.parser.kore.SymbolOrAlias
import org.kframework.backend.llvm.matching.pattern._

sealed trait Heuristic {
  def scoreAs[T](p: AsP[T], f: Fringe, c: Clause, key: Option[Pattern[Option[Occurrence]]], isEmpty: Boolean): Double = ???
  def scoreList[T](p: ListP[T], f: Fringe, c: Clause, key: Option[Pattern[Option[Occurrence]]], isEmpty: Boolean): Double = ???
  def scoreLiteral[T](p: LiteralP[T], f: Fringe, c: Clause, key: Option[Pattern[Option[Occurrence]]], isEmpty: Boolean): Double = ???
  def scoreMap[T](p: MapP[T], f: Fringe, c: Clause, key: Option[Pattern[Option[Occurrence]]], isEmpty: Boolean): Double = ???
  def scoreOr[T](p: OrP[T], f: Fringe, c: Clause, key: Option[Pattern[Option[Occurrence]]], isEmpty: Boolean): Double = ???
  def scoreSet[T](p: SetP[T], f: Fringe, c: Clause, key: Option[Pattern[Option[Occurrence]]], isEmpty: Boolean): Double = ???
  def scoreSymbol[T](p: SymbolP[T], f: Fringe, c: Clause, key: Option[Pattern[Option[Occurrence]]], isEmpty: Boolean): Double = ???
  def scoreVariable[T](p: VariableP[T], f: Fringe, c: Clause, key: Option[Pattern[Option[Occurrence]]], isEmpty: Boolean): Double = ???
  def scoreWildcard[T](p: WildcardP[T], f: Fringe, c: Clause, key: Option[Pattern[Option[Occurrence]]], isEmpty: Boolean): Double = ???

  def computeScoreForKey(c: Column, key: Option[Pattern[Option[Occurrence]]]): Double
  def breakTies(cols: Seq[(Column, Int)]): (Column, Int) = RPseudoHeuristic.breakTies(cols)
}

object Heuristic {
  def getBest(cols: Seq[(Column, Int)], allCols: Seq[(Column, Int)]): Seq[(Column, Int)] = {
    var result: List[(Column, Int)] = Nil
    var best = cols(0)._1.score
    for (col <- cols) {
      import Ordering.Implicits._
      import scala.math.max

      val bestInvalid = allCols.filter(c => !c._1.isValid && col._1.needed(c._1.keyVars)).sortBy(_._1.score).headOption
      var colBest = col._1.score

      if (!bestInvalid.isDefined) {
        colBest = col._1.score
      } else if (bestInvalid.get._1.score > colBest) {
        colBest = bestInvalid.get._1.score
      }

      if (colBest > best) {
        best = colBest
        result = col :: Nil
      } else if (colBest == best) {
        result = col :: result
      }
    }
    result
  }
}

object DefaultHeuristic extends Heuristic {
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

  def computeScoreForKey(c: Column, key: Option[Pattern[Option[Occurrence]]]): Double = {
    var result = 0.0
    for (i <- c.patterns.indices) {
      if (c.clauses(i).action.priority != c.clauses.head.action.priority)
        return result
      result += c.patterns(i).score(DefaultHeuristic, c.fringe, c.clauses(i), key, c.isEmpty)
    }
    result
  }
}

object FHeuristic extends Heuristic {
  def computeScoreForKey(c: Column, key: Option[Pattern[Option[Occurrence]]]): Double = {
    for (i <- c.patterns.indices) {
      if (c.clauses(i).action.priority != c.clauses.head.action.priority)
        return 1.0
      if (c.patterns(i).isWildcard) {
        return 0.0
      }
    }
    1.0
  }
}

object DHeuristic extends Heuristic {
  def computeScoreForKey(c: Column, key: Option[Pattern[Option[Occurrence]]]): Double = {
    -(c.patterns.count(_.isDefault))
  }
}

object BHeuristic extends Heuristic {
  def computeScoreForKey(c: Column, key: Option[Pattern[Option[Occurrence]]]): Double = {
    val sigma = c.signatureForKey(key)
    if (c.category.hasIncompleteSignature(sigma, c.fringe)) {
      -sigma.size-1
    } else {
      -sigma.size
    }
  }
}

object AHeuristic extends Heuristic {
  def computeScoreForKey(c: Column, key: Option[Pattern[Option[Occurrence]]]): Double = {
    var result = 0.0
    for (con <- c.signatureForKey(key)) {
      result -= c.fringe.expand(con).size
    }
    result
  }
}

object RHeuristic extends Heuristic {
  def computeScoreForKey(c: Column, key: Option[Pattern[Option[Occurrence]]]): Double = {
    var result = 0.0
    val signature = c.signatureForKey(key)
    for (con <- signature) {
      for (i <- c.patterns.indices) {
        if (c.patterns(i).isSpecialized(con, c.fringe, c.clauses(i))) {
          result += 1.0
        }
      }
    }

    if (c.category.hasIncompleteSignature(signature, c.fringe)) {
      for (i <- c.patterns.indices) {
        if (c.patterns(i).isDefault) {
          result += 1.0
        }
      }
    }

    -result
  }
}

object QHeuristic extends Heuristic {
  def computeScoreForKey(c: Column, key: Option[Pattern[Option[Occurrence]]]): Double = {
    var result = 0
    var priority = c.clauses.head.action.priority
    for (i <- c.patterns.indices) {
      if (c.clauses(i).action.priority != priority) {
        if (result != i) {
          return result
        }
        priority = c.clauses(i).action.priority
      }
      if (!c.patterns(i).isWildcard) {
        result += 1
      }
    }
    result
  }
}

sealed trait PseudoHeuristic extends Heuristic {
  def computeScoreForKey(c: Column, key: Option[Pattern[Option[Occurrence]]]): Double = 0.0
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
