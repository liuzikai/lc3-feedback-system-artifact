package utils

import "os"

func Exist(name string) error {
	_, err := os.Stat(name)
	if os.IsNotExist(err) {
		return err
	}
	if os.IsExist(err) {
		return nil
	}
	return err
}