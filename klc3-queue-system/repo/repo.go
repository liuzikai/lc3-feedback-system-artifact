package repo

import (
	"os"

	"github.com/go-git/go-git/v5"
	"github.com/go-git/go-git/v5/plumbing"
	githttp "github.com/go-git/go-git/v5/plumbing/transport/http"
	"github.com/sirupsen/logrus"
)

var auth = githttp.BasicAuth{
	Username: "placeholder",
	Password: "placeholder",
}

type Repo struct {
	directory string
	url       string
	repo      *git.Repository
	worktree  *git.Worktree
}

func (s *Repo) GetMasterHash() (string, string, error) {
	masterRef, err := s.repo.Reference(plumbing.NewBranchReferenceName("master"), true)
	if err != nil {
		return "", "", err
	}
	commitMsg, err := s.repo.CommitObject(masterRef.Hash())
	if err != nil {
		return "", "", err
	}
	return masterRef.Hash().String(), commitMsg.Message, nil
}
func (s *Repo) GetDirectory() string {
	return s.directory
}

func (s *Repo) init() error {
	if err := s.sync(); err != nil {
		return err
	}
	return nil
}

func (s *Repo) open() error {
	repo, err := git.PlainOpen(s.directory)
	if err != nil {
		return err
	}
	worktree, err := repo.Worktree()
	if err != nil {
		return err
	}
	s.repo = repo
	s.worktree = worktree
	return nil
}

func (s *Repo) clone() error {
	repo, err := git.PlainClone(s.directory, false, &git.CloneOptions{
		Auth:     &auth,
		URL:      s.url,
		Progress: os.Stdout,
	})
	if err != nil {
		logrus.Error("clone error", err)
		return err
	}
	worktree, err := repo.Worktree()
	if err != nil {
		return err
	}
	s.repo = repo
	s.worktree = worktree
	return nil
}

func (s *Repo) checkout(branchName string) error {
	return s.worktree.Checkout(&git.CheckoutOptions{
		Branch: plumbing.NewBranchReferenceName(branchName),
		Create: false,
		Force:  true,
	})
}

func (s *Repo) checkoutMaster() error {
	return s.checkout("master")
}

func (s *Repo) sync() error {
	if err := s.open(); err != nil {
		if err != git.ErrRepositoryNotExists {
			return err
		}
		// Then we should clone that repo
		if err := s.clone(); err != nil {
			return err
		}
	}
	// Make sure we checkout to master branch for pull
	if err := s.checkoutMaster(); err != nil {
		return err
	}
	// Repo already exist, we then should pull for latest code
	if err := s.worktree.Pull(&git.PullOptions{
		RemoteName: "origin",
		Auth:       &auth,
		Progress:   os.Stdout,
	}); err != nil && err != git.NoErrAlreadyUpToDate {
		return err
	}
	return nil
}

func (s *Repo) pull(branchName string) error {
	if err := s.worktree.Pull(&git.PullOptions{
		RemoteName:    "origin",
		ReferenceName: plumbing.NewBranchReferenceName(branchName),
		Auth:          &auth,
		Progress:      os.Stdout,
		Force:         true,
	}); err != nil && err != git.NoErrAlreadyUpToDate {
		return err
	}
	return nil
}

func (s *Repo) clear() error {
	if _, err := s.worktree.Remove("."); err != nil {
		return err
	}
	if err := s.worktree.Clean(&git.CleanOptions{Dir: true}); err != nil {
		return err
	}
	return nil
}
