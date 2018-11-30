module Main where

import           Data.Bifunctor        (second)
import qualified Data.ByteString.Char8 as B
import           Data.Map              ((!))
import           Data.Maybe            (isJust,fromJust)
import           Data.Text             (unpack,pack)
import           Data.Tuple.Select     (sel1)
import qualified Data.Yaml.Builder as Y
import           Kore.AST.Common       (Rewrites (..), SymbolOrAlias (..), Id (..))
import           Pattern               (failure,
                                        shareDt)
import           Pattern.Parser        (parseDefinition, parseTopAxioms, mainVerify,
                                        parseSymbols, getFunctions, SymLib (..),
                                        parseFunctionAxioms, AxiomInfo(..))
import           Pattern.Gen           (mkDecisionTree, mkSpecialDecisionTree)
import           System.Directory      (createDirectoryIfMissing)
import           System.Environment    (getArgs)
import           System.FilePath       (joinPath)

writeFiles :: Y.ToYaml a => FilePath -> [(String,a)] -> IO ()
writeFiles _ [] = return ()
writeFiles folder ((name, dt):files) = do
  let path = joinPath [folder, name ++ ".yaml"]
  B.writeFile path $ Y.toByteString $ Y.toYaml dt;
  writeFiles folder files

main :: IO ()
main = do
  (filename:moduleName:outputFolder:_) <- getArgs
  def <- parseDefinition filename
  createDirectoryIfMissing True outputFolder;
  let indexedMod = mainVerify def (pack moduleName)
  let axioms = parseTopAxioms def
  let symlib = parseSymbols def indexedMod
  let !dt = case axioms of
             [] -> shareDt failure
             (AxiomInfo _ _ r _):_ -> mkDecisionTree symlib indexedMod axioms [rewritesSort r]
  let specialDts = map (mkSpecialDecisionTree symlib indexedMod axioms [rewritesSort $ getRewrite $ head axioms]) axioms
  let functions = getFunctions $ symCs symlib
  let funcAxioms = fmap (parseFunctionAxioms def) functions
  let sorts = fmap (sel1 . (symCs symlib !)) functions
  let dts = zipWith (mkDecisionTree symlib indexedMod) funcAxioms sorts
  let path = joinPath [outputFolder, "dt.yaml"]
  B.writeFile path $ Y.toByteString $ Y.toYaml dt;
  let names = fmap (\(SymbolOrAlias (Id name _) _) -> unpack name) functions
  let files = zip names dts
  writeFiles outputFolder files;
  let specialNames = map (\a -> "dt_" ++ (show $ getOrdinal a)) $ axioms
  let specialFiles = zip specialNames specialDts
  let specialFiles2 = fmap (second fromJust) $ filter (isJust . snd) specialFiles
  writeFiles outputFolder specialFiles2
