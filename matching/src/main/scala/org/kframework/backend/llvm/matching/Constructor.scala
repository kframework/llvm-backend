package org.kframework.backend.llvm.matching

import org.kframework.backend.llvm.matching.pattern._
import org.kframework.parser.kore.SymbolOrAlias

sealed trait Constructor {
  def name: String
  def isBest(pat: Pattern[Option[Occurrence]]): Boolean
  def expand(f: Fringe): Option[Seq[Fringe]]
  def contract(f: Fringe, children: Seq[Pattern[String]]): Pattern[String]
}

case class Empty() extends Constructor {
  def name = "0"
  def isBest(pat: Pattern[Option[Occurrence]]): Boolean = true
  def expand(f: Fringe): Option[Seq[Fringe]] = Some(Seq())
  def contract(f: Fringe, children: Seq[Pattern[String]]): Pattern[String] = {
    val symbol = Parser.getSymbolAtt(f.symlib.sortAtt(f.sort), "unit").get
    f.sortInfo.category match {
      case SetS() => SetP(Seq(), None, symbol, SymbolP(symbol, Seq()))
      case MapS() => MapP(Seq(), Seq(), None, symbol, SymbolP(symbol, Seq()))
    }
  }
  override lazy val hashCode: Int = scala.runtime.ScalaRunTime._hashCode(this)
}

case class NonEmpty() extends Constructor {
  def name: String = ???
  def isBest(pat: Pattern[Option[Occurrence]]): Boolean = true
  def expand(f: Fringe): Option[Seq[Fringe]] = Some(Seq(f))
  def contract(f: Fringe, children: Seq[Pattern[String]]): Pattern[String] = children(0)
  override lazy val hashCode: Int = scala.runtime.ScalaRunTime._hashCode(this)
}

case class HasKey(isSet: Boolean, element: SymbolOrAlias, key: Option[Pattern[Option[Occurrence]]]) extends Constructor {
  def name = "1"
  def isBest(pat: Pattern[Option[Occurrence]]): Boolean = key.isDefined && pat == key.get
  def expand(f: Fringe): Option[Seq[Fringe]] = {
    val sorts = f.symlib.signatures(element)._1
    key match {
      case None => 
        if (isSet) {
          Some(Seq(new Fringe(f.symlib, sorts(0), Choice(f.occurrence), false), new Fringe(f.symlib, f.sort, ChoiceRem(f.occurrence), false)))
        } else {
          Some(Seq(new Fringe(f.symlib, sorts(0), Choice(f.occurrence), false), new Fringe(f.symlib, sorts(1), ChoiceValue(f.occurrence), false), new Fringe(f.symlib, f.sort, ChoiceRem(f.occurrence), false)))
        }
      case Some(k) =>
        if (isSet) {
          Some(Seq(new Fringe(f.symlib, f.sort, Rem(k, f.occurrence), false), f))
        } else {
          Some(Seq(new Fringe(f.symlib, sorts(1), Value(k, f.occurrence), false), new Fringe(f.symlib, f.sort, Rem(k, f.occurrence), false), f))
        }
    }
  }
  def contract(f: Fringe, children: Seq[Pattern[String]]): Pattern[String] = ???
  override lazy val hashCode: Int = scala.runtime.ScalaRunTime._hashCode(this)
}

case class HasNoKey(key: Option[Pattern[Option[Occurrence]]]) extends Constructor {
  def name = "0"
  def isBest(pat: Pattern[Option[Occurrence]]): Boolean = key.isDefined && pat == key.get
  def expand(f: Fringe): Option[Seq[Fringe]] = Some(Seq(f))
  def contract(f: Fringe, children: Seq[Pattern[String]]): Pattern[String] = {
    val child = children(0)
    child match {
      case MapP(keys, values, frame, ctr, orig) => 
        MapP(WildcardP[String]() +: keys, WildcardP[String]() +: values, frame, ctr, orig)
      case WildcardP() | VariableP(_, _) =>
        MapP(Seq(WildcardP[String]()), Seq(WildcardP[String]()), Some(child), Parser.getSymbolAtt(f.symlib.sortAtt(f.sort), "element").get, null)
    }
  }
  override lazy val hashCode: Int = scala.runtime.ScalaRunTime._hashCode(this)
}

case class ListC(element: SymbolOrAlias, length: Int) extends Constructor {
  def name: String = length.toString
  def isBest(pat: Pattern[Option[Occurrence]]): Boolean = true
  def expand(f: Fringe): Option[Seq[Fringe]] = {
    val sort = f.symlib.signatures(element)._1.head
    Some((0 until length).map(i => new Fringe(f.symlib, sort, Num(i, f.occurrence), false)))
  }
  def contract(f: Fringe, children: Seq[Pattern[String]]): Pattern[String] = {
    ListP(children, None, Seq(), Parser.getSymbolAtt(f.symlib.sortAtt(f.sort), "element").get, null)
  }
  override lazy val hashCode: Int = scala.runtime.ScalaRunTime._hashCode(this)
}

case class SymbolC(sym: SymbolOrAlias) extends Constructor {
  def name: String = sym.toString
  def isBest(pat: Pattern[Option[Occurrence]]): Boolean = true
  def expand(f: Fringe): Option[Seq[Fringe]] = {
    if (f.symlib.signatures(sym)._2 != f.sort) {
      None
    } else {
      val sorts = f.symlib.signatures(sym)._1
      Some(sorts.zipWithIndex.map(t => new Fringe(f.symlib, t._1, Num(t._2, f.occurrence), sym.ctr == "inj")))
    }
  }
  def contract(f: Fringe, children: Seq[Pattern[String]]): Pattern[String] = {
    SymbolP(sym, children)
  }
  override lazy val hashCode: Int = scala.runtime.ScalaRunTime._hashCode(this)
}

case class LiteralC(literal: String) extends Constructor {
  def name: String = literal
  def isBest(pat: Pattern[Option[Occurrence]]): Boolean = true
  def expand(f: Fringe): Option[Seq[Fringe]] = Some(Seq())
  def contract(f: Fringe, children: Seq[Pattern[String]]): Pattern[String] = {
    LiteralP(literal, f.sortInfo.category)
  }
  override lazy val hashCode: Int = scala.runtime.ScalaRunTime._hashCode(this)
}
