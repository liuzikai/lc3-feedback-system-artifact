package repo

import (
	"fmt"
	"github.com/sirupsen/logrus"
	"os"
	"path/filepath"
	"time"

	"github.com/go-git/go-git/v5"
	"github.com/go-git/go-git/v5/plumbing"
	"github.com/go-git/go-git/v5/plumbing/object"
	gocopy "github.com/otiai10/copy"
)

type StudentRepo struct {
	Repo
	netid string
}

func GetStudentRepo(netid string) (*StudentRepo, error) {
	directory, err := filepath.Abs(fmt.Sprintf("./assets/%s", netid))
	if err != nil {
		return nil, err
	}
	studentRepo := &StudentRepo{
		netid: netid,
	}
	studentRepo.directory = directory
	studentRepo.url = fmt.Sprintf("https://github-dev.cs.illinois.edu/ece220-fa20-zjui/%s.git", netid)

	if err := studentRepo.init(); err != nil {
		return nil, err
	}
	return studentRepo, nil
}

func (s *StudentRepo) GetGradeDir() (string, error) {
	if err := s.checkoutMaster(); err != nil {
		return "", err
	}
	return s.directory, nil
}

func (s *StudentRepo) GenerateGrade(sourceDir string, mp string, now time.Time) error {
	// Pull master again to avoid non-fast-forward
	// Make sure we checkout to master branch for pull
	if err := s.checkoutMaster(); err != nil {
		logrus.Error(err)
	} else {
		if err := s.worktree.Pull(&git.PullOptions{
			RemoteName: "origin",
			Auth:       &auth,
			Progress:   os.Stdout,
		}); err != nil && err != git.NoErrAlreadyUpToDate {
			logrus.Error(err)
		}
	}

	// Begin checkout grade
	if err := s.checkoutGrade(); err != nil {
		return err
	}

	nowStr := now.Format("2006-01-02 15:04:05")
	reportDirRelative := fmt.Sprintf("%s %s Report", nowStr, mp)
	reportDir := filepath.Join(s.directory, reportDirRelative)
	_ = os.Mkdir(reportDir, 0755)

	if err := gocopy.Copy(sourceDir, reportDir); err != nil {
		return err
	}

	if _, err := s.worktree.Add(reportDirRelative); err != nil {
		return err
	}

	if _, err := s.worktree.Commit(fmt.Sprintf("Generating report at %s", nowStr), &git.CommitOptions{
		Author: &object.Signature{
			Name: "ECE220 Feedback Tool",
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

	if err := s.checkoutMaster(); err != nil {
		return err
	}

	if err := s.worktree.Clean(&git.CleanOptions{Dir: true}); err != nil {
		return err
	}

	return nil
}

func (s *StudentRepo) checkoutGrade() error {
	if err := s.checkout("grade"); err != nil {
		if err != plumbing.ErrReferenceNotFound {
			return err
		}
		// Check for remote branch
		remoteRef, err := s.repo.Reference("refs/remotes/origin/grade", true)
		if err != nil {
			if err == plumbing.ErrReferenceNotFound {
				// We should create that on local
				if err := s.createGradeBranch(); err != nil {
					return err
				}
				return nil
			} else {
				return err
			}
		} else {
			// Then we just checkout to that remote branch
			newRef := plumbing.NewHashReference("refs/heads/grade", remoteRef.Hash())
			if err := s.repo.Storer.SetReference(newRef); err != nil {
				return err
			}
			if err := s.worktree.Checkout(&git.CheckoutOptions{
				Branch: newRef.Name(),
				Create: false,
				Keep:   false,
			}); err != nil {
				return err
			}
			if err := s.worktree.Clean(&git.CleanOptions{Dir: true}); err != nil {
				return err
			}
		}
	}
	if err := s.pullGrade(); err != nil {
		return err
	}
	return nil
}

func (s *StudentRepo) pullGrade() error {
	err := s.worktree.Pull(&git.PullOptions{
		RemoteName:    "origin",
		ReferenceName: plumbing.NewBranchReferenceName("grade"),
		Auth:          &auth,
		Progress:      os.Stdout,
		Force:         true,
	})
	if err != nil && err != git.NoErrAlreadyUpToDate {
		return err
	}
	return nil
}

func (s *StudentRepo) createGradeBranch() error {
	symRef := plumbing.NewSymbolicReference(plumbing.ReferenceName("HEAD"), plumbing.ReferenceName("refs/heads/grade"))
	if err := s.repo.Storer.SetReference(symRef); err != nil {
		return err
	}
	refs, _ := s.repo.Storer.IterReferences()
	if err := refs.ForEach(func(ref *plumbing.Reference) error {
		//logrus.Info(ref)
		return nil
	}); err != nil {
		return err
	}

	return s.clear()
}
