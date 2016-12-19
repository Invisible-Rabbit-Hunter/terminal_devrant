"use babel";

String.prototype.capitalize = function() {
    return this.charAt(0).toUpperCase() + this.slice(1);
}

module.exports = {
    cmd: "bash build.sh",
    cwd: "{PROJECT_PATH}",

    functionMatch(Output) {
        let Matches = [];
        let PrevNotNote = 0;

        const Error = /([\/0-9a-zA-Z\._]+):(\d+):(\d+): (error):\s+(.+)/;
        const Warning = /([\/0-9a-zA-Z\._]+):(\d+):(\d+): (warning):\s+(.+)/;
        const Note = /([\/0-9a-zA-Z\._]+):(\d+):(\d+): (note):\s+(.+)/;

        Output.split(/\n/).forEach((Line, LineNumber, Output) => {
            var Match = (Error.exec(Line) || Warning.exec(Line) || Note.exec(Line));
            if (Match) {
                if(Match[4] == "note")
                {
                    Matches[PrevNotNote].trace.push({
                        file: Match[1],
                        line: Match[2],
                        col: Match[3],
                        message: Match[5],
                        type: "Trace"
                    });

                    Matches.push(
                        {
                        file: Match[1],
                        line: Match[2],
                        col: Match[3],
                        message: Match[5],
                        type: "Trace"
                    });
                }
                else
                {
                    Matches.push({
                        file: Match[1],
                        line: Match[2],
                        col: Match[3],
                        message: Match[5],
                        type: Match[4].capitalize(),
                        trace: []
                    })

                    PrevNotNote = Matches.length - 1;
                }
            }
        });

        // console.log(Matches);

        return Matches;
    }
}
