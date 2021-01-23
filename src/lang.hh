/* These are predefined note names
 * sourced from respective Lilypond .ly files;
 * I have made several rather arbitrary choices if there were
 * several possibilities for one pitch; if you are native user
 * of that notation, report a divergence from widespread usage.
 *
 * Note that these hard-coded names can be still overridden by
 * (rumor-pitches) guile command
 */

const char *Notator::Langs[]={"ne","en","en-short","de","no","sv","it","ca","es",""};

const char *Notator::LangData[][35]={
	// ne
	{"ceses","ces","c","cis","cisis",
	"deses","des","d","dis","disis",
	"eses","es","e","eis","eisis",
	"feses","fes","f","fis","fisis",
	"geses","ges","g","gis","gisis",
	"ases","as","a","ais","aisis",
	"beses","bes","b","bis","bisis"},
	//en
	{"cflatflat","cflat","c","csharp","csharpsharp",
	"dflatflat","dflat","d","dsharp","dsharpsharp",
	"eflatflat","eflat","e","esharp","esharpsharp",
	"fflatflat","fflat","f","fsharp","fsharpsharp",
	"gflatflat","gflat","g","gsharp","gsharpsharp",
	"aflatflat","aflat","a","asharp","asharpsharp",
	"bflatflat","bflat","b","bsharp","bsharpsharp"},
	//en-short
	{"cff","cf","c","cs","css",
	"dff","df","d","ds","dss",
	"eff","ef","e","es","ess",
	"fff","ff","f","fs","fss",
	"gff","gf","g","gs","gss",
	"aff","af","a","as","ass",
	"bff","bf","b","bs","bss"},
	//de
	{"ceses","ces","c","cis","cisis",
	"deses","des","d","dis","disis",
	"eses","es","e","eis","eisis",
	"feses","fes","f","fis","fisis",
	"geses","ges","g","gis","gisis",
	"asas","as","a","ais","aisis",
	"heses","b","h","his","hisis"},
	//no
	{"cessess","cess","c","ciss","cississ",
	"dessess","dess","d","diss","dississ",
	"essess","ess","e","eiss","eississ",
	"fessess","fess","f","fiss","fississ",
	"gessess","gess","g","giss","gississ",
	"aessess","aess","a","aiss","aississ",
	"bess","b","h","hiss","hississ"},
	//sv
	{"cessess","cess","c","ciss","cississ",
	"dessess","dess","d","diss","dississ",
	"essess","ess","e","eiss","eississ",
	"fessess","fess","f","fiss","fississ",
	"gessess","gess","g","giss","gississ",
	"assess","ass","a","aiss","aississ",
	"hessess","b","h","hiss","hississ"},
	//it
	{"dobb","dob","do","dod","dodd",
	"rebb","reb","re","red","redd",
	"mibb","mib","mi","mid","midd",
	"fabb","fab","fa","fad","fadd",
	"solbb","solb","sol","sold","soldd",
	"labb","lab","la","lad","ladd",
	"sibb","sib","si","sid","sidd"},
	//ca
	{"dobb","dob","do","dod","dodd",
	"rebb","reb","re","red","redd",
	"mibb","mib","mi","mid","midd",
	"fabb","fab","fa","fad","fadd",
	"solbb","solb","sol","sold","soldd",
	"labb","lab","la","lad","ladd",
	"sibb","sib","si","sid","sidd"},
	//es
	{"dobb","dob","do","dos","doss",
	"rebb","reb","re","res","ress",
	"mibb","mib","mi","mis","miss",
	"fabb","fab","fa","fas","fass",
	"solbb","solb","sol","sols","solss",
	"labb","lab","la","las","lass",
	"sibb","sib","si","sis","siss"}};


