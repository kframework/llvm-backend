module Pattern.Map
  ( getMapCs
  , getMapHookName
  , isDefaultMap
  , mightUnifyMap
  , checkMapPatternIndex
  , addMapVarToRow
  , getBestMapKey
  , computeMapScore
  , getMapKeys
  , getMapVariables
  , expandMapPattern
  ) where

import Data.Functor.Foldable
       ( Fix (..) )
import Data.List
       ( maximumBy, elemIndex )
import Data.Ord
       ( comparing )
import Data.Maybe (fromJust)

import Kore.AST.Common
       ( SymbolOrAlias (..) )
import Kore.AST.MetaOrObject
       ( Object (..) )

import Pattern.Type
import Pattern.Var
import Pattern.Optimiser.Score
import Pattern

-- | Extracts the constructors from a map pattern. It also returns
-- a pattern for the next keys in the map.
getMapCs :: Column Pattern BoundPattern
         -> Clause BoundPattern
         -> Fix Pattern
         -> ([Constructor BoundPattern], Maybe (Fix Pattern))
getMapCs c cls = go
  where
    metadata :: Ignoring (Metadata BoundPattern)
    metadata  = Ignoring (getMetadata c)
    key :: Fix Pattern -> Maybe (Fix BoundPattern)
    key  = lookupCanonicalName cls
    go :: Fix Pattern -> ([Constructor BoundPattern], Maybe (Fix Pattern))
    go (Fix (MapPattern [] _ Nothing _ _))  = ([Empty], Nothing)
    go (Fix (MapPattern [] _ (Just next) _ _)) = ([], Just next)
    go p@(Fix (MapPattern (k : _) _ _ e _)) =
      ( [ HasKey False e metadata (key k)
        , HasNoKey metadata (key k)]
      , nextMap p )
    go _ = error "This should only be called on Maps"

-- | Gets the next map key if one is available or returns Nothing
-- if there are no more keys
nextMap :: Fix Pattern
        -> Maybe (Fix Pattern)
nextMap (Fix (MapPattern (_ : ks) vs f e o)) =
  if   null ks
  then Nothing
  else Just (Fix (MapPattern ks vs f e o))
nextMap _ = error "This should only be called on non-empty Maps"

-- | This is the hook name for maps. We may be able to remove this
-- one from the public interface
getMapHookName :: Maybe String
getMapHookName = Just "MAP.Map"

-- | This matches the default case for maps. We may be able to remove
-- this one from the public interface
isDefaultMap :: Clause BoundPattern -> Fix Pattern -> Bool
isDefaultMap _ (Fix (MapPattern _ _ (Just _) _ _))  = True
isDefaultMap _ (Fix (MapPattern ks vs Nothing _ _)) = not (null ks) || not (null vs)
isDefaultMap _ _ = error "This should only be called on maps"

-- | Checks if the two patterns passed as arguments are compatible
-- for unification
mightUnifyMap :: Fix BoundPattern -> Fix BoundPattern -> Bool
mightUnifyMap (Fix MapPattern{}) (Fix MapPattern{}) = True
mightUnifyMap (Fix MapPattern{}) _                  = False
mightUnifyMap _ _ = error "First argument must be a map"

checkMapPatternIndex :: (Fix BoundPattern -> Fix BoundPattern -> Bool)
                     -> Constructor BoundPattern
                     -> Metadata BoundPattern
                     -> (Clause BoundPattern, Fix Pattern)
                     -> Bool
checkMapPatternIndex _ Empty _ (_, Fix (MapPattern ks vs _ _ _)) =
  null ks && null vs
checkMapPatternIndex _ HasKey{} _ (_, Fix (MapPattern _ _ (Just _) _ _)) = True
checkMapPatternIndex f (HasKey _ _ _ (Just p)) _ (c, Fix (MapPattern ks _ Nothing _ _)) = any (f p) $ map (canonicalizePattern c) ks
checkMapPatternIndex _ (HasNoKey _ (Just p)) _ (c, Fix (MapPattern ks _ _ _ _)) =
  let canonKs = map (canonicalizePattern c) ks
  in p `notElem` canonKs
checkMapPatternIndex _ _ _ _ = error "Third argument must contain a map."

-- | Add variables bound in the pattern to the binding list
addMapVarToRow :: ( Maybe (Constructor BoundPattern) -> Occurrence -> Fix Pattern -> [VariableBinding] -> [VariableBinding] )
               -> Maybe (Constructor BoundPattern)
               -> Occurrence
               -> Fix Pattern
               -> [VariableBinding]
               -> [VariableBinding]
addMapVarToRow f _ o (Fix (MapPattern [] [] (Just p) _ _)) vars =
  f Nothing o p vars
addMapVarToRow _ _ _ (Fix MapPattern{}) vars = vars
addMapVarToRow _ _ _ _ _ = error "Fourth argument must contain a map."

-- | This function computes the score for a map.
computeMapScore :: (Metadata BoundPattern-> [(Fix Pattern, Clause BoundPattern)] -> Double)
                -> Metadata BoundPattern
                -> [(Fix Pattern, Clause BoundPattern)]
                -> Double
computeMapScore f m ((Fix (MapPattern [] [] Nothing _ _),_):tl) = 1.0 + f m tl
computeMapScore f m ((Fix (MapPattern [] [] (Just p) _ _),c):tl) = f m ((p,c):tl)
computeMapScore f m ((Fix (MapPattern ks vs _ e _),c):tl) = if f m tl == -1.0 / 0.0 then -1.0 / 0.0 else snd $ computeMapScore' f m e c ks vs tl
computeMapScore _ _ _ = error "The first pattern must be a map."

-- | This function selects the best candidate key to use when
-- computing the score for a map.
getBestMapKey :: (Metadata BoundPattern -> [(Fix Pattern, Clause BoundPattern)] -> Double)
              -> Column Pattern BoundPattern
              -> [Clause BoundPattern]
              -> Maybe (Fix BoundPattern)
getBestMapKey f (Column m (Fix (MapPattern (k:ks) vs _ e _):tl)) cs =
  fst $ computeMapScore' f  m e (head cs) (k:ks) vs (zip tl $ tail cs)
getBestMapKey _ (Column _ (Fix MapPattern{}:_)) _ = Nothing
getBestMapKey _ _ _ = error "Column must contain a map pattern."

computeMapScore' :: (Metadata BoundPattern -> [(Fix Pattern, Clause BoundPattern)] -> Double)
                 -> Metadata BoundPattern
                 -> SymbolOrAlias Object -- ^ Map.element
                 -> Clause BoundPattern
                 -> [Fix Pattern]
                 -> [Fix Pattern]
                 -> [(Fix Pattern,Clause BoundPattern)]
                 -> (Maybe (Fix BoundPattern), Double)
computeMapScore' f m e c ks vs tl =
  let zipped = zip ks vs
      scores = map (\(k,v) -> (if isBound getName c k then Just $ canonicalizePattern c k else Nothing, computeMapElementScore f m e c tl (k,v))) zipped
  in maximumBy (comparing snd) scores

-- | This function computes the score for a map when there are
-- keys and values defined.
computeMapElementScore :: (Metadata BoundPattern -> [(Fix Pattern, Clause BoundPattern)] -> Double)
                       -> Metadata BoundPattern
                       -> SymbolOrAlias Object
                       -> Clause BoundPattern
                       -> [(Fix Pattern,Clause BoundPattern)]
                       -> (Fix Pattern, Fix Pattern)
                       -> Double
computeMapElementScore f m e c tl (k,v) =
  let score = computeElementScore k c tl
  in if score == -1.0 / 0.0 then score else
  let finalScore = score * f (head $ fromJust $ getChildren m (HasKey False e (Ignoring m) Nothing)) [(v,c)]
  in if finalScore == 0.0 then minPositiveDouble else finalScore

minPositiveDouble :: Double
minPositiveDouble = encodeFloat 1 $ fst (floatRange (0.0 :: Double)) - floatDigits (0.0 :: Double)

getMapKeys :: Fix Pattern -> [Fix Pattern]
getMapKeys (Fix (MapPattern ks _ _ _ _)) = ks
getMapKeys _ = error "The getMapKeys function only support map patterns."

getMapVariables :: (Fix Pattern -> [String])
                -> Fix Pattern -> [String]
getMapVariables f (Fix (MapPattern _ _ _ _ o)) = f o
getMapVariables _ _ = error "The getMapVariables function only accepts maps."

expandMapPattern :: Constructor BoundPattern
                 -> [Metadata BoundPattern]
                 -> Metadata BoundPattern
                 -> (Fix Pattern,Clause BoundPattern)
                 -> [(Fix Pattern,Maybe (Constructor BoundPattern, Metadata BoundPattern))]
expandMapPattern (HasKey _ _ _ (Just p)) _ _ (m@(Fix (MapPattern ks vs f e o)),c) =
  let canonKs = map (canonicalizePattern c) ks
      hasKey = elemIndex p canonKs
  in case hasKey of
       Just i -> [(vs !! i,Nothing), (Fix (MapPattern (except i ks) (except i vs) f e o), Nothing), (Fix Wildcard, Nothing)]
       Nothing -> [(Fix Wildcard,Nothing), (Fix Wildcard,Nothing), (m,Nothing)]
expandMapPattern (HasNoKey _ _) _ _ (p,_) = [(p,Nothing)]
expandMapPattern _ _ _ (Fix MapPattern{},_) = error "Invalid map pattern."
expandMapPattern _ _ _ _ = error "The expandMapPattern function expects a map parameter as its final argument."

except :: Int -> [a] -> [a]
except i as =
  let (hd,tl) = splitAt i as
  in hd ++ tail tl
