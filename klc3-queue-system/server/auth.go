package server

import (
	"autograder/klc3"
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"io/ioutil"
	"net/http"
	"net/url"
	"strings"

	"github.com/gin-gonic/gin"
	"github.com/google/go-github/v32/github"
	"golang.org/x/oauth2"
)

type loginReq struct {
	ClientID     string `json:"client_id"`
	ClientSecret string `json:"client_secret"`
	Code         string `json:"code"`
}
type loginResp struct {
	AccessToken string `json:"access_token"`
	Error       string `json:"error"`
}

func codeToToken(code string) (string, error) {
	url := "https://github-dev.cs.illinois.edu/login/oauth/access_token"
	loginReq := &loginReq{
		ClientID:     "placeholder",
		ClientSecret: "placeholder",
		Code:         code,
	}
	reqJSON, err := json.Marshal(loginReq)
	if err != nil {
		return "", err
	}

	fmt.Print(string(reqJSON))
	payload := strings.NewReader(string(reqJSON))
	req, _ := http.NewRequest("POST", url, payload)
	req.Header.Add("Content-Type", "application/json")
	req.Header.Add("Accept", "application/json")
	req.Header.Add("cache-control", "no-cache")
	res, _ := http.DefaultClient.Do(req)
	defer res.Body.Close()
	body, _ := ioutil.ReadAll(res.Body)

	loginResp := &loginResp{}
	if err := json.Unmarshal(body, loginResp); err != nil {
		return "", err
	}

	if loginResp.Error != "" {
		return "", errors.New(loginResp.Error)
	}

	return loginResp.AccessToken, nil
}

func oauthHandler(c *gin.Context) {
	accessToken, err := codeToToken(c.Query("code"))
	if err != nil {
		c.Redirect(302, "https://github-dev.cs.illinois.edu/login/oauth/authorize?client_id=placeholder&scope=read:user")
		//c.JSON(400, gin.H{
		//	"message": err.Error(),
		//})
		return
	}

	ctx := context.Background()
	fmt.Printf("accessToken: %s\n", accessToken)
	ts := oauth2.StaticTokenSource(
		&oauth2.Token{AccessToken: accessToken},
	)
	tc := oauth2.NewClient(ctx, ts)

	client := github.NewClient(tc)

	client.BaseURL, _ = url.Parse("https://github-dev.cs.illinois.edu/api/v3/")

	user, _, err := client.Users.Get(ctx, "")
	if err != nil {
		c.JSON(400, gin.H{
			"message": err.Error(),
		})
		return
	}

	netid := user.Login
	name := user.Name

	c.HTML(http.StatusOK, "status.html", gin.H{
		"netid":   netid,
		"name":    name,
		"status":  klc3.GetGradeStatusAuth(*netid),
		"waiting": len(klc3.Queue),
	})

}
