<template>
    <v-app>
    <v-app-bar
      app
      color="indigo"
      dark
    >
    <v-app-bar-nav-icon @click.stop="drawer = !drawer" />
    <v-toolbar-title>LC3 Webtool @ ECE220</v-toolbar-title>
    <v-spacer></v-spacer>
      <!-- <v-btn icon large>
        <v-icon>mdi-apps</v-icon>
      </v-btn> -->
      <v-btn
          text
          rounded
          depressed
          :disabled="status=='Debug'"
          @click="statusImport"
        >
          <v-icon left dark>mdi-upload</v-icon>Import
        </v-btn>
        <v-btn
          text
          rounded
          depressed
          :disabled="status=='Debug'"
          @click="statusExport"
        >
          <v-icon left dark>mdi-download</v-icon>Export
        </v-btn>
    </v-app-bar>

  <v-navigation-drawer
      v-model="drawer"
      app
      class="indigo"
      dark
    >
    <v-list-item>
        <v-list-item-content>
          <v-list-item-title class="title">
            File System
          </v-list-item-title>
          <v-list-item-subtitle>
            LC3 file list
          </v-list-item-subtitle>
        </v-list-item-content>
      </v-list-item>

      <v-divider></v-divider>

      <v-list shaped >
        <v-list-item-group v-model="currentFile">
        <v-list-item v-for="(file, index) in fileList" @click="currentFile = index" :inactive="false" :key="index" link>
          <v-list-item-action>
            <v-icon>mdi-file</v-icon>
          </v-list-item-action>
          <v-list-item-content>
            <v-list-item-title>{{file.name}}</v-list-item-title>
          </v-list-item-content>
        </v-list-item>
        </v-list-item-group>

      </v-list>

      <!-- <template v-slot:append>
        <div class="pa-2">
          <v-btn color="deep-orange " block>New File</v-btn>
        </div>
      </template> -->

    </v-navigation-drawer>

      
    <v-main>
      <div class="home">


    <!-- <pre>
      LC3 Webtool Guides:
      Write your LC3 code in below editor, click "Compile" to run lc3as and lc3sim for your code(two steps in one click).
      If compile successfully, you should see a lightblue line indicating your current execution position. You can click on line number area to add/remove breakpoints.
      You can also click Next/Step/Continue/Finish to control lc3sim debug execution flow, which should be quite similar with lc3sim-tk.
      Just click "Compile" again if you want to rerun your code.
    </pre> -->
    <div class="control-container">
      
      <v-btn v-if="status!='Debug'" rounded color="deep-purple accent-4" :dark="true" @click="compile" :loading="status=='Compile'" ><v-icon left light >mdi-inbox</v-icon> Assemble</v-btn>
      <v-btn v-if="status=='Debug'" rounded color="pink" :dark="true" @click="terminate"><v-icon left light >mdi-stop</v-icon> Terminate</v-btn>
      <!-- <v-select
            v-model="value"
            :items="items"
            filled
            chips
            label="Chips"
            multiple
          ></v-select> -->
      <v-btn outlined color="blue-grey darken-2" dark @click="action('next')" :disabled="status!='Debug'"><v-icon left dark>mdi-redo</v-icon>Next</v-btn>
      <v-btn outlined color="blue-grey darken-2" dark @click="action('step')" :disabled="status!='Debug'"><v-icon left dark>mdi-redo</v-icon>Step</v-btn>
      <v-btn outlined color="deep-orange" dark @click="action('continue')" :disabled="status!='Debug'"><v-icon left dark>mdi-play</v-icon>Continue</v-btn>
      <v-btn outlined color="red" dark @click="action('finish')" :disabled="status!='Debug'" ><v-icon left dark>mdi-stop</v-icon>Finish</v-btn>
      <v-menu offset-y open-on-hover>
      <template v-slot:activator="{ on, attrs }">
        <v-btn
          outlined
        
          color="success"
          dark
          v-bind="attrs"
          v-on="on"
          :disabled="status!='Debug'"
        >
          Reverse
        </v-btn>
      </template>
      <v-list>
        <v-list-item @click="action('rstep')"><v-list-item-title>Reverse Step</v-list-item-title></v-list-item>
        <v-list-item @click="action('rnext')"><v-list-item-title>Reverse Next</v-list-item-title></v-list-item>
        <v-list-item @click="action('rfinish')"><v-list-item-title>Reverse Finish</v-list-item-title></v-list-item>
        <v-list-item @click="action('rcontinue')"><v-list-item-title>Reverse Continue</v-list-item-title></v-list-item>
      </v-list>
    </v-menu>
      <v-btn outlined color="indigo accent-4" dark @click="memorydump" :disabled="status!='Debug'" ><v-icon left dark>mdi-view-carousel</v-icon>Memory Dump</v-btn>
      <!-- <v-col class="d-flex" cols="12" sm="6"> -->
        <div class="select">
          <v-select
          dense
          :items="filenameList"
          v-model="fileList[currentFile].preloadList"
          label="Preload ASM List"
          outlined
          multiple
        ></v-select>
        </div>

        <!-- <v-tooltip v-if="status=='Ready'" bottom>
      <template v-slot:activator="{ on, attrs }">
        <v-btn
          color="indigo accent-4"
          dark
          rounded
          depressed
          :disabled="status!='Ready'"
          @click="preset"
          v-bind="attrs"
          v-on="on"
        >
          <v-icon left dark>mdi-view-carousel</v-icon>Import
        </v-btn>
      </template>
      <span>This action will overwrite current code</span>
    </v-tooltip> -->
    <input
        ref="uploader"
        class="d-none"
        type="file"
        accept="application/JSON"
        @change="onFileChanged"
      >
      <!-- <v-btn  color="deep-purple accent-4" :dark="true" @click="printGold" :disabled="status!='Debug'"><v-icon left light >mdi-inbox</v-icon>Lab4 Gold</v-btn> -->


        
      <!-- </v-col> -->
    </div>
     
    <div class="register-container">
        <div style="width:100% !important;" v-for="(reg, index) in regArray">
          <v-text-field
            :key="index"
            :label="regName[index]"
            outlined
            :disabled="status!='Debug'"
            dense
            style="width:90% !important;min-height: 20px !important;"
            v-model="regArray[index]"
            @change="regChange(index)"
          ></v-text-field>
        </div>
    </div>
    
    <div class="editor-container">
      <MonacoEditor class="editor" :theme="theme" :language="language" ref="editor" v-model="fileList[currentFile].code" :options="options" />
      <v-card :loading="status=='Compile'" class="output" >
        <v-card-title>
          LC3 Output
        </v-card-title>
        <v-card-text>
        
        <pre class="text" id="output">{{lc3simoutput}}</pre>
        </v-card-text>
          <!-- <template v-for="line in lc3simoutput.split('\n')"><br></template> -->
        
        
      </v-card>
    </div>
    
  </div>
    </v-main>

    <v-snackbar
      v-model="snackbar.show"
      :color="snackbar.color"
      :timeout="snackbar.timeout"
      :right="true"
      :top="true"
    >
      {{ snackbar.text }}

      <!-- <template v-slot:action="{ attrs }">
        <v-btn
          color="pink"
          text
          v-bind="attrs"
          @click="snackbar = false"
        >
          Close
        </v-btn>
      </template> -->
    </v-snackbar>

      <v-dialog v-model="dialog" fullscreen hide-overlay transition="dialog-bottom-transition">
      <v-card>
        <v-toolbar dark color="primary">
          <v-btn icon dark @click="dialog = false">
            <v-icon>mdi-close</v-icon>
          </v-btn>
          <v-toolbar-title>Memory Dump</v-toolbar-title>
          <v-spacer></v-spacer>
          <v-toolbar-items>
            <v-text-field
            style="align-items: center;"
            v-model="addrSearch"
            clearable
            flat
            solo-inverted
            hide-details
            :error="searchError"
            prepend-inner-icon="mdi-view-carousel"
            label="Jump to Address"
            @keydown.enter="searchMemory"
          ></v-text-field>
          </v-toolbar-items>
          <!-- <v-toolbar-items>
            <v-btn dark text @click="dialog = false">Go to PC</v-btn>
          </v-toolbar-items> -->
        </v-toolbar>
        <v-list three-line subheader>
          <v-subheader>Memeory Table</v-subheader>
          <v-list-item>
              <v-data-table
    :headers="headers"
    :items="memoryData"
    :options.sync="options"
    :items-per-page="15"
    :server-items-length="65536"
    :search="addrSearch"
    dense
  >
  <template v-slot:item.label="{ item }">
      <v-chip v-if="item.label" small :color="'success'" dark>{{ item.label }}</v-chip>
    </template>
  </v-data-table>
          </v-list-item>
          <!-- <v-list-item>
            <v-list-item-content>
              <v-list-item-title>Content filtering</v-list-item-title>
              <v-list-item-subtitle>Set the content filtering level to restrict apps that can be downloaded</v-list-item-subtitle>
            </v-list-item-content>
          </v-list-item>
          <v-list-item>
            <v-list-item-content>
              <v-list-item-title>Password</v-list-item-title>
              <v-list-item-subtitle>Require password for purchase or use password to restrict purchase</v-list-item-subtitle>
            </v-list-item-content>
          </v-list-item> -->
        </v-list>
        <!-- <v-divider></v-divider> -->
        
        </v-list>
      </v-card>
    </v-dialog>

    <v-footer
      :color="statusColor[status]"
      app
    >
      <div class="white--text footspan">
        <div>
          <b>{{status}} {{fileList[currentFile].name}}</b>
        </div>
        <div>
          <b>
            {{statusMsg}}
          </b>
          
        </div>
          
      </div>
    </v-footer>
  </v-app>


  
</template>

<script>
// @ is an alias to /src
import MonacoEditor from 'vue-monaco'
import * as wasm from '@/util/wasm'
import * as lc3 from '@/util/lc3'
import * as lab from '@/util/lab'

export default {
  name: 'Home',
  components: {
    MonacoEditor
  },
  data() {
    return {
      headers: [
          {
            text: 'Address(Hex)',
            align: 'center',
            sortable: false,
            value: 'address',
            width: "100"
          },
          { text: 'Label', value: 'label', sortable: false, align: 'center', width: "150" },
          { text: 'Value(Hex)', value: 'value', sortable: false, align: 'center', width: "140" },
          { text: 'Instruction', value: 'instruction', sortable: false, width: "240" },
        ],
        memoryData: [],
      addrSearch: "",
      dialog: false,
      drawer: false,
      status: "Ready",
      statusMsg: "",
      editor: null,
      outputKey: 0,
      dumped: false,
      firsthalt: true,
      lc3asModule: null,
      lc3simModule: null,
      lc3simoutput: "",
      nextInput: "",
      lineNum: 0,
      preloadList: [],
      decorations: [],
      fileList: [],
      theme: "",
      language: "",
      currentFile: 0,
      decorationStr: [],
      searchError: false,
      options: {

      },
      inputPromiseResolve: null,
      snackbar: {
        show: false,
        color: "success",
        text: "sb",
        timeout: 2000
      },
      debugMap: {},
      regArray: [...Array(11)].map(item => this.num2hex(0)),
      breakPoints: {},  // line -> breakpoint
      regName: ["R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7", "PC", "IR", "PSR"],
      options: {
        glyphMargin: true
      },
      statusColor: {
        Ready: "blue darken-1",
        Debug: "orange darken-3",
        Compile: "purple darken-2",
        Error: "red darken-1"
      },
      code: ``,
      initFile: [{
          name: 'main.asm',
          code: "; comment here",
          sym: null,
          obj: null,
          lc3output: "",
          lc3error: "",
          lc3asResult: null,
          preloadList: []

        },
        {
          name: 'file1.asm',
          code: "",
          sym: null,
          obj: null,
          lc3output: "",
          lc3error: "",
          lc3asResult: null,
          preloadList: []
        },
        {
          name: 'file2.asm',
          code: "",
          sym: null,
          obj: null,
          lc3output: "",
          lc3error: "",
          lc3asResult: null,
          preloadList: []
        },
      ]
    }
  },
  computed: {
    filenameList: function() {
      return this.fileList.map(item => item.name).filter(item => item != this.fileList[this.currentFile].name)
    }
  },
  created() {
    console.log(wasm)
    this.fileList = this.getFileStore(window.localStorage.getItem("code")) || this.initFile;
    setInterval(() => {
      window.localStorage.setItem("code", this.dumpJSON());
    }, 3000)
    global.getInput = async(buffer) => {
      // console.log(buffer);
      this.inputPromise = new Promise((resolve, reject) => {
        this.inputPromiseResolve = resolve;
      });
      const command = await this.inputPromise;
      console.log("we got a promise command", command)
      // const command = prompt("Please input some command");
      const input = wasm.toUTF8StrArray(command);
      this.lc3simModule.HEAPU8.set(input, buffer); // write WASM memory calling the set method of the Uint8Array
    }
    global.setLineNum = (lineNum) => {
      this.lineNum = lineNum;
      this.switchLine();
    }
    global.reportDDR = (charCode) => {
      // console.log("charCode", )
      this.lc3simoutput += String.fromCharCode(charCode);
      this.updateScroll();
    }
    global.setRegisters = (lc3Register) => {
      console.log(lc3Register)
      const regArray = new Int32Array(this.lc3simModule.HEAP32.buffer, lc3Register, 11);
      this.regArray = Array.from(regArray).map(item => this.num2hex(item));
      if(this.regArray[8] == "x0494") {
        if(this.firsthalt) {
          this.firsthalt = false;
          return;
        }
        this.showWarn("halting the LC-3")
      }
      // console.log(this.regArray)
    }
    global.setDebugInfo = (lc3Debug) => {
      this.status = "Debug";
      this.debugMap = {};
      const debugArray = new Int32Array(this.lc3simModule.HEAP32.buffer, lc3Debug, 65536);
      for(let i = 0; i < debugArray.length; i++) {
        if(debugArray[i] != 0) {
          this.debugMap[debugArray[i]] = i;
        }
      }
      console.log(this.debugMap)
    }
    global.setLineInfo = (label, op, oprands) => {
      const labelStr = wasm.ptr2str(this.lc3simModule, label)
      const labelArr = labelStr.split(" ");
      const opStr = wasm.ptr2str(this.lc3simModule, op)
      const oprandsStr = wasm.ptr2str(this.lc3simModule, oprands)
      this.statusMsg = `${labelArr.length == 3 ? labelArr[0] + " :" : ''} ${opStr} ${oprandsStr}`;
    }
    global.reportSucc = (info) => {
      const infoStr = wasm.ptr2str(this.lc3simModule, info);
      this.showSucc(infoStr);
    }
    global.reportErr = (info) => {
      const infoStr = wasm.ptr2str(this.lc3simModule, info);
      this.showErr(infoStr);
    }
    global.reportWarn = (info) => {
      const infoStr = wasm.ptr2str(this.lc3simModule, info);
      this.showWarn(infoStr);
    }
  },
  mounted() {
    this.editor = this.$refs.editor.getEditor();
    lc3.init(this.editor);
    this.theme = "myCoolTheme";
    this.language = "mySpecialLanguage";
    global.editor = this.editor;
    this.decorations = [];
    this.editor.onMouseDown((event) => {
      if(event.target.type != 3 && event.target.type != 2)
        return;
      const lineNumber = event.target.position.lineNumber;
      if(this.breakPoints[lineNumber]) {
        // remove breakpoint
        // console.log("remove", lineNumber);
        this.breakPoints[lineNumber] = false;
        this.decorations = this.decorations.filter((item) => {
          return item.type != "breakpoint" || (item.type == "breakpoint" && item.lineNumber != lineNumber);
        });
        this.inputPromiseResolve(`break clear #${this.debugMap[lineNumber]}`)
      } else {
        // add breakpoint
        if(this.debugMap[lineNumber] == undefined)
          return;
        this.breakPoints[lineNumber] = true;
        this.decorations.push({
          lineNumber,
          range: new monaco.Range(lineNumber,1,lineNumber,1),
          type: "breakpoint",
          options: {
              isWholeLine: true,
              glyphMarginClassName: 'breakpoint'
          }
        });
        this.inputPromiseResolve(`break set #${this.debugMap[lineNumber]}`)
        
      }
      this.updateDecoration();
    })
  },
  watch: {
      options: {
        handler () {
          this.getMemoryDumpData()
        },
        deep: true,
      },
      // addrSearch: {
      //   handler () {
      //     const startAddr = parseInt(Number("0x" + this.addrSearch.replace(/^\x|X+/g, '')), 10);
      //     if(Number.isNaN(startAddr) == NaN || startAddr >= 35536)
      //       return;
      //     this.options.page = Math.floor(startAddr/this.options.itemsPerPage)+1;
      //     // console.log(this.options.page)
      //     // this.getMemoryDumpData()
      //   }
      // }
  },
  methods: {
    dumpJSON() {
      return JSON.stringify(this.fileList.map(item => {
        return {
          name: item.name,
          code: item.code,
          preloadList: item.preloadList
        }
      }));
    },
    readFile(file) {
      return new Promise((resolve, reject) => {
        const reader = new FileReader();
        reader.onload = function() {
          resolve(reader.result);
        };
        reader.readAsText(file);
      })
    },
    async onFileChanged(e) {
      const file = e.target.files[0];
      if(file) {
        if(this.fileList[this.currentFile].code != "") {
          this.download(`lc3webtool_checkpoint_${new Date().getTime()}.json`, this.dumpJSON())
        }
        const content = await this.readFile(file);
        this.fileList = this.getFileStore(content);
      }
    },
    download(filename, text) {
      var element = document.createElement('a');
      element.setAttribute('href', 'data:text/plain;charset=utf-8,' + encodeURIComponent(text));
      element.setAttribute('download', filename);

      element.style.display = 'none';
      document.body.appendChild(element);

      element.click();

      document.body.removeChild(element);
    },
    statusImport() {
      // console.log(lab.lab3())
      this.$refs.uploader.click();
      // this.fileList[this.currentFile].code = lab.lab4();
    },
    statusExport() {
      this.download(`lc3webtool_export_${new Date().getTime()}.json`, this.dumpJSON());
    },
    searchMemory() {
      // console.log(this.addrSearch)
      const startAddr = parseInt(Number("0x" + this.addrSearch.replace(/^\x|X+/g, '')), 10);
      // console.log(startAddr)
      if(Number.isNaN(startAddr) || startAddr >= 35536) {
        this.searchError = true;
        return;
      }
      this.searchError = false;
      this.options.page = Math.floor(startAddr/this.options.itemsPerPage)+1;
    },
    memorydump() {

      // this.searchError = false;
      this.dialog = true;

      if(this.dumped)
        return;
      this.dumped = true;

      const startAddr = parseInt(Number("0x" + this.regArray[8].replace(/^\x|X+/g, '')), 10);
      console.log(this.regArray[8], startAddr)
      if(Number.isNaN(startAddr) || startAddr >= 35536) {
        return;
      }
      this.$nextTick(() => {
        this.options.page = Math.floor(startAddr/this.options.itemsPerPage)+1;
      });

      // setTimeout(() => {
        

      // }, 1000)

      
      // this.getMemoryDumpData();
    },
    getMemoryDumpData() {
      // console.log(regArray[8] +i, wasm.disasOne(this.lc3simModule, regArray[8] + i))
      const { page, itemsPerPage } = this.options;
      let startAddr = (page-1) * itemsPerPage;
      if(startAddr >= 35536)
        return;
      // console.log(this.options)
      // console.log(itemsPerPage, page)
      
      this.memoryData = [...Array(itemsPerPage)].map((item, idx) => {
        const data = wasm.disasOne(this.lc3simModule, startAddr+idx);
        const labelArr = data.label.split(" ");
        return {
          address: this.num2hex(startAddr+idx),
          label: labelArr.length == 3 ? labelArr[0] : "",
          value: labelArr.slice(-1).join(" "),
          instruction: `${data.op} ${data.operands}`
        }
      })
      // console.log(this.memoryData)
    },
    getFileStore(file) {
      try {
        if(file) {
          return JSON.parse(file).map((item) => {
            return {
              name: "",
              code: "",
              preloadList: [],
              ...item,
              sym: null,
              obj: null,
              lc3output: "",
              lc3error: "",
              lc3asResult: null
            }
          });
        } else {
          return null;
        }
      } catch (err) {
        console.log(err);
        return null;
      }
    },
    showSucc(info) {
      this.snackbar.show = true;
      this.snackbar.text = info;
      this.snackbar.color = "success";
      this.snackbar.timeout = 2000;
    }, 
    showErr(info) {
      this.snackbar.show = true;
      this.snackbar.text = info;
      this.snackbar.color = "error";
      this.snackbar.timeout = 3000;
    },
    showWarn(info) {
      this.snackbar.show = true;
      this.snackbar.text = info;
      this.snackbar.color = "warning";
      this.snackbar.timeout = 3000;
    },
    regChange(idx) {
      console.log(`register ${this.regName[idx]} ${this.regArray[idx]}`)
      this.inputPromiseResolve(`register ${this.regName[idx]} ${this.regArray[idx]}`)
    },
    updateDecoration() {
      // console.log(this.decorations)
      this.decorationStr = this.editor.deltaDecorations(this.decorationStr, this.decorations);
    },
    clearBreakpoint() {
      this.debugMap = {};
      this.breakPoints = {};
      this.decorations = [];
      this.decorationStr = this.editor.deltaDecorations(this.decorationStr, []);
    },  
    addBreakPoint() {

    },
    num2hex(num) {
      return "x" + num.toString(16).padStart(4, '0').toUpperCase();
    },
    switchLine() {
      if (this.lineNum != 0) {
        const lineNumber = this.lineNum;
        this.decorations = this.decorations.filter((item) => {
          return item.type != "hit";
        });
        this.decorations.push({
          lineNumber,
          range: new monaco.Range(lineNumber,1,lineNumber,1),
          type: "hit",
          options: {
              isWholeLine: true,
              className: 'myContentClass',
              inlineClassName: 'myInlineDecoration'
          }
        })
        this.updateDecoration();
      } else {
        this.decorations = this.decorations.filter((item) => {
          return item.type != "hit";
        });
        this.updateDecoration();
      }
      this.editor.revealLineInCenter(this.lineNum);
      
    },
    action(name) {
      console.log(name);
      this.inputPromiseResolve(name)
    },
    terminate() {
      this.status = "Ready";
      // this.inputPromiseResolve("finish")
      this.clearBreakpoint();
    },
    updateScroll() {
      this.$nextTick(() => {
        const container = this.$el.querySelector("#output");
        container.scrollTop = container.scrollHeight;
      });
    },
    async lc3asFile(file) {
      file.lc3output = "";
      file.lc3error = "";
      const lc3asModule = await createLC3asModule({
        print: (text) => { 
          file.lc3output += (text + "\n");
        },
        printErr: (text) => { file.lc3error += text + "\n"; file.lc3output += text + "\n";}
      });
      file.lc3asResult = wasm.runlc3as(lc3asModule, file.code);
      
    },
    async initPreloadFile() {
      
    },
    getRealName(name) {
      return name.split(".")[0];
    },
    async compile() {
      this.firsthalt = true;
      this.dumped = false;
      this.status = "Compile";
      this.statusMsg = "";
      this.clearBreakpoint();
      this.lc3simoutput = "";
      this.editor.revealLineInCenter(1);
      // we should get our depend list first
      const curFile = this.fileList[this.currentFile];
      console.log("compile", curFile.preloadList)
      const totalFile = [curFile, ...this.fileList.filter(item => curFile.preloadList.includes(item.name))];
      const totalFileName = totalFile.map(item => item.name);
      console.log("totalFile", totalFile)
      console.log("totalFileName", totalFileName)

      for(let i = 0; i < this.fileList.length; i++) {
        const file = this.fileList[i];
        if(!totalFileName.includes(file.name))
          continue;
        await this.lc3asFile(file);
        if(file.lc3asResult.ret != 0) {
          this.status = "Error"
          this.statusMsg = file.lc3error;
          this.currentFile = i;
          this.lc3simoutput = file.lc3output;
          this.showErr(`${file.lc3error}`);
          return;
        }
      }


      // Init lc3sim first
      this.lc3simModule = await createLC3simModule({
        print: (text) => { this.lc3simoutput += text + "\n"; this.updateScroll(); },
        printErr: (text) => { this.lc3simoutput += text + "\n"; this.updateScroll(); }
      });
      const FS = this.lc3simModule.FS;
      for(let i = 0; i < this.fileList.length; i++) {
        const file = this.fileList[i];
        if(!totalFileName.includes(file.name))
          continue;
        FS.writeFile(`${this.getRealName(file.name)}.sym`, file.lc3asResult.sym);
        FS.writeFile(`${this.getRealName(file.name)}.obj`, file.lc3asResult.obj);
        if(file.name == curFile.name)
          FS.writeFile(`${this.getRealName(file.name)}.debug`, file.lc3asResult.debug);
      }

      const mainFile = this.fileList[this.currentFile];
      this.lc3simoutput = mainFile.lc3output;
      const targetFile = [mainFile, ...this.fileList.filter(item => totalFileName.includes(item.name) && item.name !=  mainFile.name)].reverse()
      const lc3simResult = wasm.runlc3sim(this.lc3simModule, targetFile.map(item => `${this.getRealName(item.name)}.obj` ));
      console.log(lc3simResult)
      
      
      // this.$forceUpdate();

      // this.$nextTick(() => {
      //   this.$nextTick(() => {
          
          
      //   })
      // })
      
    }
  }

}
</script>
<style lang="less">
.control-container {
  margin: 0px 0px -10px 0px;
  display: flex;
  flex-direction: row;
  justify-content: left;
  // align-items: center;
  button {
    margin: 0px 5px;
  }
  .select {
    margin: -3px 5px 0px 10px;
  }
}
.home {
  margin: 20px;
}
.editor-container {
  
  display: flex;
  flex-direction: row;
.editor {
  width: 80vw;
  height: calc(100vh - 270px);
}
.output {
  
  width: 80%;
  
  margin-left: 20px;
  .text {
    font-family: 'Fira Code','Courier New', Courier, monospace;
    height: calc(100vh - 370px);
    overflow: scroll;
  }
}
}

.breakpoint {
  background: red;
  border-radius: 50%;
  display: inline-block;
  width: 14px !important;
  height: 14px !important;
}
.myContentClass {
	background: rgb(235,50,35);
	// background: rgb(8, 193, 255);
}
.myInlineDecoration {
	color: white !important;
	// cursor: pointer;
	font-weight: bold;
	// font-style: oblique;
}

.register-container {
  margin: 0px 0px 0px 0px;
  display: flex;
  flex-direction: row;
  justify-content: space-between;
  input {
    width: 30px;
  }
}

.footspan {
  width: 100%;
  display: flex;
  flex-direction: row;
  justify-content: space-between;
}


</style>