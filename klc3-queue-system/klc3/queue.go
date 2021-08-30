package klc3

import (
	"autograder/repo"
	"autograder/utils"
	"fmt"
	"io/ioutil"
	"os"
	"path/filepath"
	"strings"
	"sync"
	"time"

	gocopy "github.com/otiai10/copy"

	"github.com/sirupsen/logrus"

	"gopkg.in/go-playground/webhooks.v5/github"
)

type GradeTask struct {
	Netid      string
	IsManual   bool
	ManualList []string
	Payload    *github.PushPayload
}

type GradeStatus struct {
	Running bool
	LastRun time.Time
	Error   error
}

var Queue chan *GradeTask
var gradeStatusMap map[string]*GradeStatus
var mutex sync.RWMutex
var commitUrl = "[%s](https://github-dev.cs.illinois.edu/ece220-fa20-zjui/%s/tree/%s)\n\nCommit Message: %s"
var netidWhiteList = map[string]bool{"wenqing4": true, "qili8": true, "tingkai2": true, "zikail2": true, "lumetta": true}

func GetGradeStatus() *map[string]GradeStatus {
	statusCopy := make(map[string]GradeStatus)
	mutex.RLock()
	for key, value := range gradeStatusMap {
		statusCopy[key] = *value
	}
	mutex.RUnlock()
	return &statusCopy
}

func GetGradeStatusAuth(netid string) *GradeStatus {
	var status GradeStatus
	mutex.RLock()
	if statusPtr, ok := gradeStatusMap[netid]; ok {
		status = *statusPtr
	}
	mutex.RUnlock()
	return &status
}

func GetQueueStatus() {

}

func genTmpDir() (string, error) {
	tmpDir, err := filepath.Abs(("./tmp"))
	if err != nil {
		return "", err
	}
	_ = os.Mkdir(tmpDir, 0755)
	return ioutil.TempDir(tmpDir, "output")
}

func extractConcrete(outputDir string, hash string, concreteStoreDir string) error {
	// MKDir for target
	if err := os.MkdirAll(concreteStoreDir, 0755); err != nil {
		return err
	}
	// Extract vector set if we have any
	outputConcreteDir := filepath.Join(outputDir, "store")
	defer os.RemoveAll(outputConcreteDir)
	if err := utils.Exist(outputConcreteDir); err != nil {
		// No concrete found, we do not extratct
		return nil
	}

	// Then we copy to klc3 storage
	if err := gocopy.Copy(outputConcreteDir, concreteStoreDir, gocopy.Options{
		Skip: func(src string) (bool, error) {
			ext := filepath.Ext(src)
			if ext == ".obj" || ext == ".sym" || ext == ".var" || ext == ".info" {
				return true, nil
			}
			return false, nil
		},
	}); err != nil {
		return err
	}
	logrus.Infof("extractor concrete to %s succ", concreteStoreDir)
	return nil
}

func gradeMP(MP string, gradeTask *GradeTask, studentRepo *repo.StudentRepo) error {
	directory, err := studentRepo.GetGradeDir()
	if err != nil {
		return err
	}
	// Get hash
	hash, commitMsg, err := studentRepo.GetMasterHash()
	if err != nil {
		return err
	}

	// Gen output dir
	outputDir, err := genTmpDir()
	if err != nil {
		return err
	}
	defer os.RemoveAll(outputDir)

	// Run klc3
	concreteDir, err := filepath.Abs(filepath.Join(repo.Klc3StoragePath, gradeTask.Netid, strings.ToLower(MP)))
	if err != nil {
		return err
	}
	begin := time.Now()
	if err := os.MkdirAll(concreteDir, 0755); err != nil {
		return err
	}
	err = MPHandler(MP, directory, outputDir, concreteDir)
	if err != nil {
		return err
	}
	runningTime := time.Now().Sub(begin)
	logrus.Infof("Running klc3 done for %s, hash: %s, commit_msg: %s, time: %s", gradeTask.Netid, hash, commitMsg, runningTime.String())

	// Prepare for README.md
	now := time.Now()
	nowStr := now.Format("2006-01-02 15:04:05")

	concreteStoreDir := filepath.Join(concreteDir, hash)
	repo.Klc3StorageMutex.RLock()
	if err := extractConcrete(outputDir, hash, concreteStoreDir); err != nil {
		logrus.Error(err) // note: here we just output log, but don't return
	}
	repo.Klc3StorageMutex.RUnlock()

	// Write file
	reportByte, err := ioutil.ReadFile(filepath.Join(outputDir, "README.md"))
	if err != nil {
		return err
	}
	report := string(reportByte)
	report = strings.Replace(report, "{{TIME}}", nowStr, -1)
	report = strings.Replace(report, "{{COMMIT_ID_AND_LINK}}", fmt.Sprintf(commitUrl, hash, gradeTask.Netid, hash, commitMsg), -1)

	// Write to README.md
	if err := ioutil.WriteFile(filepath.Join(outputDir, "README.md"), []byte(report), 0644); err != nil {
		return err
	}
	logrus.Infof("%s report generated for %s", MP, gradeTask.Netid)

	if err := studentRepo.GenerateGrade(outputDir, MP, now); err != nil {
		return err
	}

	logrus.Infof("%s report pushed for %s", MP, gradeTask.Netid)
	return nil

}

func genMPList(gradeTask *GradeTask) []string {
	result := make([]string, 0, 6)
	if gradeTask.IsManual {
		result = append(result, gradeTask.ManualList...)
	} else {
		for _, commit := range gradeTask.Payload.Commits {
			if utils.StringInSlice("mp/mp1/mp1.asm", commit.Added) || utils.StringInSlice("mp/mp1/mp1.asm", commit.Modified) {
				result = append(result, "MP1")
			}
			if utils.StringInSlice("mp/mp2/mp2.asm", commit.Added) || utils.StringInSlice("mp/mp2/mp2.asm", commit.Modified) {
				result = append(result, "MP2")
			}
			// if _, ok := netidWhiteList[gradeTask.Netid]; ok {
			if utils.StringInSlice("mp/mp3/mp3.asm", commit.Added) || utils.StringInSlice("mp/mp3/mp3.asm", commit.Modified) {
				result = append(result, "MP3")
			}
			// }
		}
	}

	return result
}

func grade(gradeTask *GradeTask) error {
	studentRepo, err := repo.GetStudentRepo(gradeTask.Netid)
	if err != nil {
		logrus.Error("Fetch studentRepo fail", err)
		return err
	}

	mpList := genMPList(gradeTask)
	logrus.Infof("Grading %s for %+v", gradeTask.Netid, mpList)

	for _, MP := range mpList {
		// Then we start mp related grading
		if err := gradeMP(MP, gradeTask, studentRepo); err != nil {
			return err
		}
	}

	return nil
}

func StartQueue(consumerCount int, chanSize int, waitTime time.Duration) error {
	Queue = make(chan *GradeTask, chanSize)
	gradeStatusMap = make(map[string]*GradeStatus)
	for i := 1; i <= consumerCount; i++ {
		go func(id int) {
			logrus.Infof("Consumer %d started", id)
			for gradeTask := range Queue {
				// We first check for file change / manual
				if len(genMPList(gradeTask)) == 0 {
					logrus.Warnf("grader ignore commit for no change %s - %s", gradeTask.Netid, gradeTask.Payload.HeadCommit.ID)
					continue
				}

				shouldRun := true
				var lastRun time.Time
				mutex.Lock()
				gradeStatus, ok := gradeStatusMap[gradeTask.Netid]
				if !ok {
					// Init new status
					now := time.Now()
					gradeStatusMap[gradeTask.Netid] = &GradeStatus{
						Running: true,
						LastRun: now,
					}
					shouldRun = true
				} else {
					if gradeStatus.Running {
						shouldRun = false
					} else {
						if gradeTask.IsManual || time.Now().Sub(gradeStatus.LastRun) >= waitTime {
							shouldRun = true
						} else {
							shouldRun = false
						}
					}
				}
				lastRun = gradeStatusMap[gradeTask.Netid].LastRun
				gradeStatusMap[gradeTask.Netid].Error = nil
				if shouldRun {
					gradeStatusMap[gradeTask.Netid].LastRun = time.Now()
					gradeStatusMap[gradeTask.Netid].Running = true
				}
				mutex.Unlock()

				if !shouldRun {
					// grader is running for current task, we ignore that grading request
					logrus.Warnf("grader is running/waiting %s, lastRun = %+v, we ignore that grading request", gradeTask.Netid, lastRun)
					continue
				}

				var err error
				if err = grade(gradeTask); err != nil {
					logrus.Error(err)
				}

				mutex.Lock()
				gradeStatusMap[gradeTask.Netid].Running = false
				gradeStatusMap[gradeTask.Netid].Error = err
				mutex.Unlock()
			}
		}(i)
	}

	return nil
}
