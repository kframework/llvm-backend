{-# LANGUAGE FlexibleInstances #-}
{-# LANGUAGE OverloadedStrings #-}
{-# LANGUAGE InstanceSigs      #-}

module Main where

import           Data.Bits             (shiftL)
import           Data.Functor.Foldable (Fix (..))
import           Data.List             (transpose)
import           Data.Map.Strict       (fromList, (!))
import           Data.Proxy            (Proxy (..))
import           Data.Semigroup        ((<>))

import           Test.Tasty            (TestTree, defaultMain, testGroup)
import           Test.Tasty.HUnit      (testCase, (@?=))

import           Pattern               hiding (getMetadata)
import           Pattern.Class

data IntPat = IntLit Int
            | IntWld
            | IntVar String
            deriving (Show, Eq)

data Lst  = Cns IntPat Lst -- index 1
          | Nil     -- index 0
          | Wld     -- wildcard
          | Var String
          deriving (Show, Eq)

instance IsPattern Lst where
  toPattern :: Lst -> Fix Pattern
  toPattern (Cns i l) = Fix (Pattern "cons" Nothing [toPattern i, toPattern l])
  toPattern Nil     = Fix (Pattern "nil" Nothing  [])
  toPattern Wld     = Fix Wildcard
  toPattern (Var v) = Fix (Variable v)

instance IsPattern IntPat where
  toPattern :: IntPat -> Fix Pattern
  toPattern (IntLit i) = Fix (Pattern (show i) (Just 32) [])
  toPattern IntWld     = Fix Wildcard
  toPattern (IntVar v) = Fix (Variable v)

instance HasMetadata IntPat where
  getMetadata :: Proxy IntPat -> Metadata
  getMetadata _ = Metadata (shiftL 1 32, f)
    where
      f :: String -> [Metadata]
      f _ = []

instance HasMetadata Lst where
  getMetadata :: Proxy Lst -> Metadata
  getMetadata _ =
    let m = fromList
                    [ ("nil", []) -- Nil
                    , ("cons", [ getMetadata (Proxy :: Proxy IntPat)
                               , getMetadata (Proxy :: Proxy Lst)
                               ]) -- Cns Lst (1)
                    ]
    in Metadata (length m, (!) m)

mkLstPattern :: [[Lst]] -> (ClauseMatrix, [Occurrence])
mkLstPattern ls =
  let as = take (length ls) [1..]
      md = getMetadata (Proxy :: Proxy Lst)
      cs = fmap (Column md . (toPattern <$>)) (transpose ls)
  in case mkClauseMatrix cs as of
       Right matrix -> matrix
       Left  msg    -> error $ "Invalid definition: " ++ show msg

defaultPattern :: (ClauseMatrix, [Occurrence])
defaultPattern =
  mkLstPattern [ [Nil, Wld]
               , [Wld, Nil]
               , [Wld, Wld] ]

appendPattern :: (ClauseMatrix, [Occurrence])
appendPattern =
  mkLstPattern [ [Nil, Wld]
               , [Wld, Nil]
               , [Cns IntWld Wld, Cns IntWld Wld] ]

appendBindPattern :: (ClauseMatrix, [Occurrence])
appendBindPattern =
  mkLstPattern [ [Nil, Var "as"]
               , [Var "bs", Nil]
               , [Cns (IntVar "b") (Var "bs"), Cns (IntVar "a") (Var "as")] ]

matchHeadPattern :: (ClauseMatrix, [Occurrence])
matchHeadPattern =
  mkLstPattern [ [Cns (IntLit 0) Wld]
               , [Cns (IntLit 1) Wld]
               , [Cns (IntLit (-1)) Wld]
               , [Cns (IntLit 1000000) Wld] ]

tests :: TestTree
tests = testGroup "Tests" [appendTests]

appendTests :: TestTree
appendTests = testGroup "Basic pattern compilation"
  [ testCase "Naive compilation of the append pattern" $
      compilePattern appendPattern @?=
        switch [ ("nil", leaf 1 [])
               , ("cons", simplify (simplify
                           (switch [ ("nil", leaf 2 [])
                                   , ("cons", leaf 3 [])
                                   ] Nothing )))
               ] Nothing
  , testCase "Naive compilation of the append pattern with variable bindings" $
      compilePattern appendBindPattern @?=
        switch [ ("nil", leaf 1 [[2]])
               , ("cons", simplify (simplify
                           (switch [ ("nil", leaf 2 [[1]])
                                   , ("cons", leaf 3 [[0, 2], [1, 2], [0, 1], [1, 1]])
                                   ] Nothing )))
               ] Nothing
  , testCase "Yaml serialization" $
      (serializeToYaml $ compilePattern $ appendBindPattern) @?= 
        "specializations:\n" <>
        "- - nil\n" <>
        "  - action:\n" <>
        "    - 1\n" <>
        "    - - - 2\n" <>
        "- - cons\n" <>
        "  - specializations: []\n" <>
        "    default:\n" <>
        "      specializations: []\n" <>
        "      default:\n" <>
        "        specializations:\n" <>
        "        - - nil\n" <>
        "          - action:\n" <>
        "            - 2\n" <>
        "            - - - 1\n" <>
        "        - - cons\n" <>
        "          - action:\n" <>
        "            - 3\n" <>
        "            - - - 0\n" <>
        "                - 2\n" <>
        "              - - 1\n" <>
        "                - 2\n" <>
        "              - - 0\n" <>
        "                - 1\n" <>
        "              - - 1\n" <>
        "                - 1\n" <>
        "        default: null\n" <>
        "default: null\n"
  , testCase "Naive compilation of integer literal patterns" $
      compilePattern matchHeadPattern @?=
        switch [ ("cons", (switchLit [ ("0", leaf 1 [])
                                     , ("1", leaf 2 [])
                                     , ("-1", leaf 3 [])
                                     , ("1000000", leaf 4 [])
                                     ] 32 (Just failure) ))
               ] (Just failure)
  ]
{-compileTests :: TestTree
compileTests = testGroup "Compiling Kore to Patterns"
  [ testCase "Compilation of imp.kore" $
     parseDefinition "imp.kore" @?= parseDefinition "imp.kore"
  ]-}

main :: IO ()
main = defaultMain tests
