package klc3

import (
	"autograder/utils"
	"path/filepath"
)

type MP int

const (
	MP1 MP = iota
	MP2
	MP3
)


func MPHandler(MP string, repoDir string, outputDir string, concreteDir string) error {
	mpFile := filepath.Join(repoDir, utils.MPList[MP])
	if err := utils.Exist(mpFile); err != nil {
		return err
	}
	if err := ExecMP(MP, mpFile, outputDir, concreteDir); err != nil {
		return err
	}
	return nil
}
