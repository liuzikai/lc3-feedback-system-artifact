package repo

import (
	"fmt"
	"path/filepath"
	"sync"
	"time"

	"github.com/go-git/go-git/v5"
	"github.com/go-git/go-git/v5/plumbing/object"
	"github.com/sirupsen/logrus"
)

type Klc3Repo struct {
	Repo
	// TODO: Check whether we need add RW lock here
}

var Klc3StoragePath = fmt.Sprintf("./assets/%s", "klc3Storage")
var Klc3StorageMutex sync.RWMutex

func GetKlc3Repo() (*Klc3Repo, error) {
	directory, err := filepath.Abs(Klc3StoragePath)
	if err != nil {
		return nil, err
	}
	klc3Repo := &Klc3Repo{}
	klc3Repo.directory = directory
	klc3Repo.url = "https://github-dev.cs.illinois.edu/wenqing4/klc3Storage.git"

	if err := klc3Repo.init(); err != nil {
		return nil, err
	}
	return klc3Repo, nil
}

func (s *Klc3Repo) AutoSyncExe() error {
	// We check for modification
	status, err := s.worktree.Status()
	if err != nil {
		return err
	}
	if status.IsClean() {
		return nil
	}

	if err := s.pull("master"); err != nil {
		logrus.Error(err)
		// TODO: Try to push here (maybe last commit not pushed)
		s.repo.Push(&git.PushOptions{
			RemoteName: "origin",
			Auth:       &auth,
		})
		return err
	}

	if _, err := s.worktree.Add("."); err != nil {
		return err
	}

	now := time.Now()
	if _, err := s.worktree.Commit(fmt.Sprintf("Sync vector set at %s", now.Format("2006-01-02 15:04:05")), &git.CommitOptions{
		Author: &object.Signature{
			Name: "ECE220 Autograder",
			//Email: "wenqing4@illinois.edu",
			When: now,
		},
	}); err != nil {
		return err
	}

	if err := s.repo.Push(&git.PushOptions{
		RemoteName: "origin",
		Auth:       &auth,
	}); err != nil {
		return err
	}
	logrus.Info("Klc3Storage AutoSynced")
	return nil
}

func (s *Klc3Repo) AutoSync() error {
	go func() {
		logrus.Info("Klc3Storage AutoSync started")
		for {
			Klc3StorageMutex.Lock()
			if err := s.AutoSyncExe(); err != nil {
				logrus.Error(err)
			}
			Klc3StorageMutex.Unlock()
			time.Sleep(time.Second * 10)
		}
	}()

	return nil
}
