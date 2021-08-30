export function init() {
    // Register a new language
    console.log("monaco", monaco)
    monaco.languages.register({
        id: 'mySpecialLanguage'
    });

    // Register a tokens provider for the language
    monaco.languages.setMonarchTokensProvider('mySpecialLanguage', {
        tokenizer: {
            root: [
                [/^\s*(ldr|LDR|ldi|LDI|ld|LD|str|STR|sti|STI|st|ST|lea|LEA|and|AND|add|ADD|not|NOT)/, "operator"],
                [/\s+(ldr|LDR|ldi|LDI|ld|LD|str|STR|sti|STI|st|ST|lea|LEA|and|AND|add|ADD|not|NOT)(\s|;|$)/, "operator"],
                [/^\s*(BR|br)[nN]?[zZ]?[pP]?\b/, "control"],
                [/\s+(BR|br)[nN]?[zZ]?[pP]?(\s|;|$)/, "control"],
                [/^\s*(jmp|JMP|jsrr|JSRR|jsr|JSR|ret|RET|trap|TRAP|getc|GETC|out|OUT|puts|PUTS|in|IN|putsp|PUTSP|halt|HALT)\b/, "control"],
                [/\s+(jmp|JMP|jsrr|JSRR|jsr|JSR|ret|RET|trap|TRAP|getc|GETC|out|OUT|puts|PUTS|in|IN|putsp|PUTSP|halt|HALT)(\s|;|$)/, "control"],
                [/\b[rR][0-7]\b/, "register"],
                [/[xX][0-9a-fA-F]{1,4}/, "numeric"],
                [/#\d+/, "numeric"],
                [/[01]{8}/, "numeric"],
                [/\.\w*/, "directives"],
                [/;.*$/, "comment"]
            ]
        }
    });

    // Define a new theme that contains only rules that match this language
    monaco.editor.defineTheme('myCoolTheme', {
        base: 'vs',
        inherit: false,
        rules: [{
            token: 'operator',
            foreground: '0000FF'
        },
        {
            token: 'control',
            foreground: 'FF0000'
        },
        {
            token: 'register',
            foreground: '004080'
        },
        {
            token: 'numeric',
            foreground: '098658'
        },
        {
            token: 'directives',
            foreground: '795E26'
        },
        {
            token: 'comment',
            foreground: '008000'
        }
        ]
    });

    // Register a completion item provider for the new language
    monaco.languages.registerCompletionItemProvider('mySpecialLanguage', {
        provideCompletionItems: () => {
            var suggestions = [{
                label: 'simpleText',
                kind: monaco.languages.CompletionItemKind.Text,
                insertText: 'simpleText'
            }, {
                label: 'testing',
                kind: monaco.languages.CompletionItemKind.Keyword,
                insertText: 'testing(${1:condition})',
                insertTextRules: monaco.languages.CompletionItemInsertTextRule.InsertAsSnippet
            }, {
                label: 'ifelse',
                kind: monaco.languages.CompletionItemKind.Snippet,
                insertText: [
                    'if (${1:condition}) {',
                    '\t$0',
                    '} else {',
                    '\t',
                    '}'
                ].join('\n'),
                insertTextRules: monaco.languages.CompletionItemInsertTextRule.InsertAsSnippet,
                documentation: 'If-Else Statement'
            }];
            return {
                suggestions: suggestions
            };
        }
    });

    // monaco.editor.setTheme('myCoolTheme')
    // monaco.editor.setLanguage('mySpecialLanguage')
}