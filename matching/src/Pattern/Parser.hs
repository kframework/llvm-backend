{-|
Module      : Pattern.Parser
Description : Parses rewrite rules and functions from Kore definitions.
Copyright   : (c) Runtime Verification, 2018
License     : UIUC/NCSA
Maintainer  : radu.ometita@iohk.io
Stability   : experimental
Portability : POSIX
-}
module Pattern.Parser (
  -- * Symbols indexing
  -- $symbols
    SymLib(..)
  , parseSymbols
  -- * Axiom Patterns
  -- $axioms
  , AxiomPattern(..)
  , PatternWithSideCondition(..)
  , getAxiomSideCondition
  , getAxiomPattern
  , findFunctionSymbols
  , getTopChildren
  , unifiedPatternRAlgebra
  , parseTopAxioms
  , parseFunctionAxioms
  , parseDefinition
  , mainVerify
  ) where

import           Control.Monad
                 ( guard, (>=>) )
import           Data.Functor.Foldable
                 ( Fix (..), para )
import           Data.Functor.Impredicative
                 ( Rotate31 (..) )
import           Data.List
                 ( nub, sortBy )
import           Data.List.Index
                 ( indexed )
import qualified Data.Map as Map
import           Data.Maybe
                 ( catMaybes, mapMaybe, fromMaybe )
import           Data.Ord
                 ( comparing )
import           Data.Proxy
                 ( Proxy (..) )

import           Kore.AST.Common
                 ( And (..), Application (..), Ceil (..), Equals (..),
                 Exists (..), Floor (..), Forall (..), Id (..), Iff (..),
                 Implies (..), In (..), Next (..), Not (..), Or (..),
                 Pattern (..), Rewrites (..), Sort (..), SymbolOrAlias (..),
                 Top (..), Variable )
import           Kore.AST.Kore
                 ( CommonKorePattern, pattern KoreObjectPattern, KorePattern,
                 UnifiedPattern (..), UnifiedSortVariable )
import           Kore.AST.MetaOrObject
                 ( Meta (..), Object (..), Unified (..) )
import           Kore.AST.Sentence
                 ( Attributes (..), Definition (..), KoreDefinition,
                 Module (..), ModuleName (..), Sentence (..),
                 SentenceAxiom (..), applyUnifiedSentence )
import           Kore.ASTHelpers
                 ( ApplicationSorts (..) )
import           Kore.ASTVerifier.DefinitionVerifier
                 ( defaultAttributesVerification, verifyAndIndexDefinition )
import qualified Kore.Builtin as Builtin
import           Kore.Error
                 ( printError )
import           Kore.IndexedModule.IndexedModule
                 ( KoreIndexedModule )
import           Kore.IndexedModule.MetadataTools
                 ( MetadataTools (..), extractMetadataTools )
import           Kore.Parser.Parser
                 ( fromKore )
import           Kore.Step.StepperAttributes
                 ( StepperAttributes (..) )

--[ Metadata ]--

data SymLib = SymLib
  { symCs :: Map.Map (SymbolOrAlias Object) ([Sort Object], Sort Object, StepperAttributes)
  , symSt :: Map.Map (Sort Object) [SymbolOrAlias Object]
  } deriving (Show, Eq)

isLiteralSymbol :: SymbolOrAlias Object -> Bool
isLiteralSymbol (SymbolOrAlias _ params) = all isLiteralSort params
  where
    isLiteralSort :: Sort Object -> Bool
    isLiteralSort (SortActualSort _)   = True
    isLiteralSort (SortVariableSort _) = False

parseAxiomForSymbols :: SentenceAxiom UnifiedSortVariable UnifiedPattern Variable
                     -> [SymbolOrAlias Object]
parseAxiomForSymbols = parsePatternForSymbols . sentenceAxiomPattern
  where
    parsePatternForSymbols :: CommonKorePattern -> [SymbolOrAlias Object]
    parsePatternForSymbols = para (unifiedPatternRAlgebra (const []) rAlgebra)
    rAlgebra :: Pattern Object Variable (CommonKorePattern,
                                     [SymbolOrAlias Object])
             -> [SymbolOrAlias Object]
    rAlgebra (AndPattern (And _ (_, p₀) (_, p₁)))         = p₀ ++ p₁
    rAlgebra (ApplicationPattern (Application s ps))      =
      let parameterSymbols = (mconcat $ map snd ps)
      in  if isLiteralSymbol s
          then s : parameterSymbols
          else parameterSymbols
    rAlgebra (CeilPattern (Ceil _ _ (_, p)))              = p
    rAlgebra (EqualsPattern (Equals _ _ (_, p₀) (_, p₁))) = p₀ ++ p₁
    rAlgebra (ExistsPattern (Exists _ _ (_, p)))          = p
    rAlgebra (FloorPattern (Floor _ _ (_, p)))            = p
    rAlgebra (ForallPattern (Forall _ _ (_, p)))          = p
    rAlgebra (IffPattern (Iff _ (_, p₀) (_, p₁)))         = p₀ ++ p₁
    rAlgebra (ImpliesPattern (Implies _ (_, p₀) (_, p₁))) = p₀ ++ p₁
    rAlgebra (InPattern (In _ _ (_, p₀) (_, p₁)))         = p₀ ++ p₁
    rAlgebra (NextPattern (Next _ (_, p)))                = p
    rAlgebra (NotPattern (Not _ (_, p)))                  = p
    rAlgebra (OrPattern (Or _ (_, p₀) (_, p₁)))           = p₀ ++ p₁
    rAlgebra (RewritesPattern (Rewrites _ (_, p₀) (_, p₁))) = p₀ ++ p₁
    rAlgebra _                                            = []

mkSymLib :: [SymbolOrAlias Object]
         -> MetadataTools Object StepperAttributes
         -> SymLib
mkSymLib symbols metaTools =
  let (SymLib sorts syms) = foldl go (SymLib Map.empty Map.empty) symbols
  in SymLib sorts (Map.map nub syms)
  where
    go (SymLib dIx rIx) symbol =
      let as = (sortTools metaTools) symbol
          att = (symAttributes metaTools) symbol
          args = applicationSortsOperands as
          result = applicationSortsResult as
      in SymLib { symCs = Map.insert symbol (args, result, att) dIx
                , symSt = Map.insert result (symbol : (Map.findWithDefault [] result rIx)) rIx
                }

parseSymbols :: KoreDefinition -> KoreIndexedModule StepperAttributes -> SymLib
parseSymbols def indexedMod =
  let axioms = getAxioms def
      symbols = mconcat (parseAxiomForSymbols <$> axioms)
      metaTools = extractMetadataTools indexedMod
  in mkSymLib symbols metaTools

--[ Patterns ]--

getSentenceAttributes :: SentenceAxiom UnifiedSortVariable UnifiedPattern Variable
                      -> [String]
getSentenceAttributes (sentenceAxiomAttributes -> Attributes attrs) =
  catMaybes ((matchObject >=> getAttributeId) <$> attrs)
  where
    matchObject :: CommonKorePattern
                -> Maybe (Pattern Object Variable CommonKorePattern)
    matchObject (Fix (UnifiedPattern (UnifiedObject obj))) = Just (unRotate31 obj)
    matchObject _ = Nothing
    getAttributeId :: Pattern Object Variable CommonKorePattern -> Maybe String
    getAttributeId
      (ApplicationPattern (Application (SymbolOrAlias (Id aId _) _) _)) = Just aId
    getAttributeId _ = Nothing

hasSentenceAttribute :: SentenceAxiom UnifiedSortVariable UnifiedPattern Variable
                     -> String
                     -> Bool
hasSentenceAttribute sentence attribute =
  any (attribute ==) (getSentenceAttributes sentence)

viewSentenceAttribute :: String
                      -> SentenceAxiom UnifiedSortVariable UnifiedPattern Variable
                      -> Bool
viewSentenceAttribute = flip hasSentenceAttribute

rulePriority :: SentenceAxiom UnifiedSortVariable UnifiedPattern Variable
             -> Int
rulePriority (viewSentenceAttribute "owise" -> True) = 3
rulePriority (viewSentenceAttribute "cool"  -> True) = 2
rulePriority (viewSentenceAttribute "heat"  -> True) = 1
rulePriority _                    = 0

data PatternWithSideCondition pat =
  PatternWithSideCondition { getPattern :: pat
                           , getSideCondition :: Maybe CommonKorePattern
                           }

data AxiomPattern pat = AxiomPattern
                        { getPriority                 :: Int
                        , getOrdinal                  :: Int
                        , getPatternWithSideCondition :: PatternWithSideCondition pat
                        }

getAxiomPattern :: AxiomPattern pat -> pat
getAxiomPattern = getPattern . getPatternWithSideCondition

getAxiomSideCondition :: AxiomPattern pat -> Maybe CommonKorePattern
getAxiomSideCondition = getSideCondition . getPatternWithSideCondition

parseAxiomSentence :: (CommonKorePattern -> Maybe (PatternWithSideCondition pat))
                   -> (Int, SentenceAxiom UnifiedSortVariable UnifiedPattern Variable)
                   -> Maybe (AxiomPattern pat)
parseAxiomSentence split (ix, sentence) = do
  PatternWithSideCondition rewrite sideCondition <-
    split (sentenceAxiomPattern sentence)
  guard (any (hasSentenceAttribute sentence) skipAttrs)
  return $ AxiomPattern { getPriority = (rulePriority sentence)
                        , getOrdinal = ix
                        , getPatternWithSideCondition =
                            PatternWithSideCondition rewrite sideCondition
                        }
  where
    skipAttrs = ["comm", "assoc", "idem"]

unifiedPatternRAlgebra :: (Pattern Meta variable (CommonKorePattern, b) -> b)
                       -> (Pattern Object variable (CommonKorePattern, b) -> b)
                       -> (UnifiedPattern variable (CommonKorePattern, b) -> b)
unifiedPatternRAlgebra metaT _ (UnifiedPattern (UnifiedMeta meta)) =
  metaT (unRotate31 meta)
unifiedPatternRAlgebra _ objectT (UnifiedPattern (UnifiedObject object)) =
  objectT (unRotate31 object)

matchFunction :: SymbolOrAlias Object
              -> CommonKorePattern
              -> Maybe (PatternWithSideCondition (Equals Object CommonKorePattern))
matchFunction symbol topPattern =
  case topPattern of
    SImplies _ (SEquals _ _ pat _)
               (SAnd _ eq@(SEquals _ _ (SApp s _) _) _) ->
      if s == symbol
      then Just $ PatternWithSideCondition (extract eq) (Just pat)
      else Nothing
    SImplies _ (STop _)
               (SAnd _ eq@(SEquals _ _ (SApp s _) _) _) ->
      if s == symbol
      then Just $ PatternWithSideCondition (extract eq) Nothing
      else Nothing
    SImplies _ (SAnd _ _ (SEquals _ _ pat _))
               (SAnd _ eq@(SEquals _ _ (SApp s _) _) _) ->
      if s == symbol
      then Just $ PatternWithSideCondition (extract eq) (Just pat)
      else Nothing
    SImplies _ (SAnd _ _ (STop _))
               (SAnd _ eq@(SEquals _ _ (SApp s _) _) _) ->
      if s == symbol
      then Just $ PatternWithSideCondition (extract eq) Nothing
      else Nothing
    eq@(SEquals _ _ (SApp s _) _) ->
      if s == symbol
      then Just $ PatternWithSideCondition (extract eq) Nothing
      else Nothing
    _ -> Nothing
  where
    extract :: KorePattern v -> Equals Object (KorePattern v)
    extract (KoreObjectPattern (EqualsPattern eq)) = eq
    -- This is only called after matching `EqualsPattern` so matching should never reach this pattern
    extract _                                      = undefined

matchTop :: CommonKorePattern
         -> Maybe (PatternWithSideCondition (Rewrites Object CommonKorePattern))
matchTop topPattern =
  case topPattern of
    SAnd _ (SEquals _ _ pat _)
           (SAnd _ _ rw@(SRewrites _ _ _)) ->
      Just $ PatternWithSideCondition (extract rw) (Just pat)
    SAnd _ (STop _)
           (SAnd _ _ rw@(SRewrites _ _ _)) ->
      Just $ PatternWithSideCondition (extract rw) Nothing
    _ -> Nothing
  where
    extract :: KorePattern v -> Rewrites Object (KorePattern v)
    extract (KoreObjectPattern (RewritesPattern rw)) = rw
    -- This is only called after matching `RewritesPattern` so matching should never reach this pattern
    extract _                                        = undefined

-- |'getAxiom' retrieves the 'SentenceAxiom' patterns from a KoreDefinition
getAxioms :: KoreDefinition
          -> [SentenceAxiom UnifiedSortVariable UnifiedPattern Variable]
getAxioms koreDefinition =
  let modules   = definitionModules koreDefinition
      sentences = mconcat (moduleSentences <$> modules)
  in mconcat ((applyUnifiedSentence metaT metaT) <$> sentences)
  where
    metaT :: Sentence lvl UnifiedSortVariable UnifiedPattern Variable
          -> [SentenceAxiom UnifiedSortVariable UnifiedPattern Variable]
    metaT = \case
      SentenceAxiomSentence s -> [s]
      _ -> []

parseTopAxioms :: KoreDefinition
               -> [AxiomPattern (Rewrites Object CommonKorePattern)]
parseTopAxioms koreDefinition =
  let axioms = getAxioms koreDefinition
      withIndex = indexed axioms
      withOwise = mapMaybe (parseAxiomSentence matchTop) withIndex
  in sortBy (comparing getPriority) withOwise

parseFunctionAxioms :: KoreDefinition
                    -> SymbolOrAlias Object
                    -> [AxiomPattern (Equals Object CommonKorePattern)]
parseFunctionAxioms koreDefinition symbol =
  let axioms = getAxioms koreDefinition
      withIndex = indexed axioms
      withOwise = mapMaybe (parseAxiomSentence (matchFunction symbol)) withIndex
  in sortBy (comparing getPriority) withOwise

parseDefinition :: FilePath -> IO KoreDefinition
parseDefinition fileName = do
  contents  <- readFile fileName
  let result = fromKore fileName contents
  case result of
    Left err         -> error err
    Right definition -> return definition

mainVerify
    :: KoreDefinition
    -> String
    -> KoreIndexedModule StepperAttributes
mainVerify definition mainModuleName =
    let attributesVerification = defaultAttributesVerification Proxy
        verifyResult = verifyAndIndexDefinition
                       attributesVerification
                       Builtin.koreVerifiers
                       definition
    in case verifyResult of
         Left  err     -> error (printError err)
         Right modules ->
           fromMaybe (error "Could not find main module") $
                     Map.lookup (ModuleName mainModuleName) modules

hasFunctionalAttributes :: (SymbolOrAlias Object,
                            ([Sort Object], Sort Object, StepperAttributes))
                        -> Bool
hasFunctionalAttributes (_, (_, _, attributes)) = isFunction attributes

-- Return the list of symbols that are actually functions.
findFunctionSymbols :: Map.Map (SymbolOrAlias Object)
                        ([Sort Object], Sort Object, StepperAttributes)
                    -> [SymbolOrAlias Object]
findFunctionSymbols signatures =
  fst <$> filter hasFunctionalAttributes (Map.assocs signatures)

getTopChildren :: CommonKorePattern -> [CommonKorePattern]
getTopChildren (SApp _ ps) = ps
getTopChildren _ = error "Unexpected pattern on lhs of function"

-- [ Patterns ]

pattern SImplies :: Sort Object -> KorePattern v -> KorePattern v -> KorePattern v
pattern SImplies sort fst snd = KoreObjectPattern (ImpliesPattern (Implies sort fst snd))

pattern SApp :: SymbolOrAlias Object -> [KorePattern v] -> KorePattern v
pattern SApp symbolOrAlias cs = KoreObjectPattern (ApplicationPattern (Application symbolOrAlias cs))

pattern SAnd :: Sort Object -> KorePattern v -> KorePattern v -> KorePattern v
pattern SAnd sort left right <- KoreObjectPattern (AndPattern (And sort left right))

pattern SEquals :: Sort Object -> Sort Object -> KorePattern var -> KorePattern var -> KorePattern var
pattern SEquals operandSort resultSort fst snd <- KoreObjectPattern (EqualsPattern (Equals operandSort resultSort fst snd))

pattern SRewrites :: Sort Object -> KorePattern var -> KorePattern var -> KorePattern var
pattern SRewrites sort fst snd = KoreObjectPattern (RewritesPattern (Rewrites sort fst snd))

pattern STop :: Sort Object -> KorePattern var
pattern STop sort = KoreObjectPattern (TopPattern (Top sort))
