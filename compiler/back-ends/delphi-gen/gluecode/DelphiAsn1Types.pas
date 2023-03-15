{
@abstract(Delphi Asn1 "basic" types)
@author(CHS)
@created(14.07.2016)
@lastmod(09.01.2018)

Description:
  basic types (integer, string, ...) for delphi units created by the snacc compiler.
  In this first implementation only capable of doing json en-/decoding.

  Hints / answers from stm regarding code generation:
  * timestamps are transfereed as UTC always
  * enum transfered as integer, for use of programming languge the enum type is generated
  * octetstring is transfered base64 encoded always
  * octetcontaining is an octetstring with compiler hint to contain utf8 and is usually transferred as utf8string

History:
    CHS 14.07.2016 Created file
    CHS 21.07.2016 Added types for time, octet
    CHS 27.07.2016 Added function GetJsonValue
    CHS 28.07.2016 Added type TAsnSeqOf
    CHS 29.07.2016 Added type AsnNull
    CHS 10.10.2016 Added Support for newer Delphi versions
    CHS 03.04.2017 Added AsnTime.Clear() (generated code makes use of this function)
    CHS 09.01.2018 Fixed warning in Delphi 10.2 Tokyo
}


unit DelphiAsn1Types;

interface
uses
  System.Sysutils,
  System.DateUtils,
  System.Generics.Collections,
  System.TimeSpan,
  // starting with Delphi XE6 the TJSON* classes reside in System.JSON
  {$IF CompilerVersion >= 27}
  System.JSON,
  System.NetEncoding,
  {$ELSE}
  Data.DBXJson,
  Soap.EncdDecd,  // for base64
  {$IFEND}

  Soap.XsBuiltins; // for   TXSDateTime / ISO8601 datetime conversion

type
  EAsnException = class (Exception);

  EAsnTagMissingException = class(EAsnException)
  private
    FTypeName: String;
    FPropertyName: String;
  public
    constructor Create(const ATypename, APropertyName: String);
    property TypeName: String read FTypeName;
    property PropertyName: String read FPropertyName;
  end;

  EAsnTagDecodeException = class(EAsnException)
  private
    FTypeName: String;
    FPropertyName: String;
  public
    constructor Create(const ATypename, APropertyName: String);
    property TypeName: String read FTypeName;
    property PropertyName: String read FPropertyName;
  end;

  TAsnBase = class
    constructor Create(); virtual;
    procedure JEnc(var json: TJSONValue); virtual; abstract;
    function JDec(json: TJSONValue; suppressErrMissing: Boolean = false; suppressErrDecode: Boolean = false): Boolean; virtual; abstract;
  end;
  TAsnBaseClass = class of TAsnBase;

  AsnBool = class
  public
    value: Boolean;
    constructor Create(const bValue: Boolean = false);

    //procedure Clear();
    procedure JEnc(var jValue : TJSONValue);
    function JDec(const jValue : TJSONValue): Boolean;
  end;
  //PAsnBool = ^AsnBool;

  AsnOctet = class
  public
    value: RawByteString;
    constructor Create(const sValue: RawByteString = '');

    //procedure Clear();
    procedure JEnc(var jValue : TJSONValue);
    function JDec(const jValue : TJSONValue): Boolean;
  end;
  //PAsnOctet = ^AsnOctet;

  AsnTime = class
  protected
    FUTCOffset: String;
  public
    value: TDateTime;
    constructor Create(const xValue: TDateTime = 0.0);

    procedure Clear();
    procedure JEnc(var jValue : TJSONValue);
    function JDec(const jValue : TJSONValue): Boolean;
  end;
  //PAsnTime = ^AsnTime;

//  AsnUtcTime = class(AsnTime)
//  public
//    constructor Create(const xValue: TDateTime = 0.0);
//  end;
//
//  AsnSystemTime = class(AsnTime)
//  public
//    constructor Create(const xValue: TDateTime = 0.0);
//  end;

  AsnInteger = class
  public
    value: Integer;
    constructor Create(const iValue: Integer = 0);

    //procedure Clear();
    procedure JEnc(var jValue : TJSONValue);
    function JDec(const jValue : TJSONValue): Boolean;
  end;
  //PAsnInteger = ^AsnInteger;

  AsnReal = class
  public
    value: Double;
    constructor Create(const rValue: Double = 0);

    //procedure Clear();
    procedure JEnc(var jValue : TJSONValue);
    function JDec(const jValue : TJSONValue): Boolean;
  end;
  //PAsnReal = ^AsnReal;

  AsnUtf8String = class
  public
    value: String;
    constructor Create(const sValue: String = '');

    //procedure Clear();
    procedure JEnc(var jValue : TJSONValue);
    function JDec(const jValue : TJSONValue): Boolean;
  end;
  //PAsnUtf8String = ^AsnUtf8String;

  AsnNull = class
  public
    procedure JEnc(var jValue : TJSONValue);
    function JDec(const jValue : TJSONValue): Boolean;
  end;

  TAsnSeqOf<T: class> = class(TObjectList<T>)

  end;

function GetJsonValue(const jo: TJsonObject; const sName: String; var jsonValue: TJSONValue): Boolean;

implementation

function GetJsonValue(const jo: TJsonObject; const sName: String; var jsonValue: TJSONValue): Boolean;
var
  jp: TJsonPair;
begin
  Result := false;
  jsonValue := nil;
  jp := jo.Get(sName);
  if Assigned(jp) then
  begin
    jsonValue := jp.JsonValue;
    result := Assigned(jsonValue);
  end;
end;

{ EAsnInvalidTagException }

constructor EAsnTagDecodeException.Create(const ATypename,
  APropertyName: String);
begin
  inherited Create(ATypename + ' decode failed: ' + APropertyName);
  FTypeName := ATypename;
  FPropertyName := APropertyName;
end;

{ EAsnTagMissingException }

constructor EAsnTagMissingException.Create(const ATypename,
  APropertyName: String);
begin
  inherited Create(ATypename + ' not found: ' + APropertyName);
  FTypeName := ATypename;
  FPropertyName := APropertyName;
end;

{ AsnUtf8String }

constructor AsnUtf8String.Create(const sValue: String);
begin
  inherited Create();
  self.value := sValue;
end;

function AsnUtf8String.JDec(const jValue: TJSONValue): Boolean;
begin
  result := false;
  if jValue is TJsonString then
  begin
    self.value := TJsonString(jValue).Value;
    result := true;
  end;
end;

procedure AsnUtf8String.JEnc(var jValue: TJSONValue);
begin
  jValue := TJSONString.Create(self.value);
end;

{ AsnBool }

constructor AsnBool.Create(const bValue: Boolean);
begin
  inherited Create();
  self.value := bValue;
end;


function AsnBool.JDec(const jValue: TJSONValue): Boolean;
begin
  result := (jValue is TJSONTrue) or (jValue is TJSONFalse);
  self.value := jValue is TJSONTrue;
end;

procedure AsnBool.JEnc(var jValue: TJSONValue);
begin
  if self.value then
    jValue := TJSONTrue.Create()
  else
    jValue := TJSONFalse.Create();
end;

{ AsnInteger }

constructor AsnInteger.Create(const iValue: Integer);
begin
  inherited Create();
  self.value := iValue;
end;

function AsnInteger.JDec(const jValue: TJSONValue): Boolean;
begin
  result := false;
  if jValue is TJsonNumber then
  begin
    self.value := TJSONNumber(jValue).AsInt;
    result := true;
  end;
end;

procedure AsnInteger.JEnc(var jValue: TJSONValue);
begin
  jValue := TJSONNumber.Create(self.value);
end;

{ AsnReal }

constructor AsnReal.Create(const rValue: Double);
begin
  inherited Create();
  self.value := rValue;
end;

function AsnReal.JDec(const jValue: TJSONValue): Boolean;
begin
  result := false;
  if jValue is TJsonNumber then
  begin
    self.value := TJSONNumber(jValue).AsDouble;
    result := true;
  end;
end;

procedure AsnReal.JEnc(var jValue: TJSONValue);
begin
  jValue := TJSONNumber.Create(self.value);
end;

{ AsnTime }

procedure AsnTime.Clear;
begin
  self.value := 0;
end;

constructor AsnTime.Create(const xValue: TDateTime);
begin
  inherited Create();
  FUtcOffset := 'Z'; // estos always transfers times as UTC
  self.value := xValue;

end;

function AsnTime.JDec(const jValue: TJSONValue): Boolean;
var
  tmp:  TXSDateTime;
begin
  result := false;
  // from http://stackoverflow.com/questions/1438870/in-delphi-is-there-a-function-to-convert-xml-date-and-time-to-tdatetime/1439154#1439154
  if jValue is TJsonString then
  begin

    tmp := TXSDateTime.Create();
    try
      tmp.XSToNative(TJsonString(jValue).Value);
      self.value := tmp.AsUtcDateTime; // always UTC. if jValue is 17:00:00+2:00 value is set to 15:00:00
      result := true;
    except
      result := false;
    end;
    tmp.Free;
  end;

end;

procedure AsnTime.JEnc(var jValue: TJSONValue);
var
  LResult: String;
begin
  LResult := FormatDateTime('yyyy''-''mm''-''dd''T''hh'':''nn'':''ss', self.Value) + FUTCOffset;
//  LResult := FormatDateTime('yyyy''-''mm''-''dd''T''hh'':''nn'':''ss''.''zzz', self.Value) + FUTCOffset;

  jValue := TJSONString.Create(LResult);
end;

{ AsnOctet }

constructor AsnOctet.Create(const sValue: RawByteString);
begin
  inherited Create();
  self.value := sValue;
end;

function AsnOctet.JDec(const jValue: TJSONValue): Boolean;
var
  rslt: TBytes;
begin
  result := false;
  if jValue is TJsonString then
  begin
{$IF CompilerVersion >= 27}
    rslt := TNetEncoding.Base64.DecodeStringToBytes(TJsonString(jValue).Value);
{$ELSE}
    rslt := DecodeBase64(AnsiString(TJsonString(jValue).Value)); // TJsonString(jValue).Value is assumed to only contain BASE64-chars (subset of US-ASCII 7 bit)
{$IFEND}
    // convert tbytes to RawByteString
    setLength(self.value, Length(rslt));
    move(rslt[0], self.value[1], Length(rslt));

    result := true;
  end;
end;

procedure AsnOctet.JEnc(var jValue: TJSONValue);
begin

  // EncodeBase64 adds line end every 72(?) characters - remove them
  jValue := TJSONString.Create(
        StringReplace(
{$IF CompilerVersion >= 27}
            TNetEncoding.Base64.EncodeBytesToString(@(self.value[1]), length(self.value)),
{$ELSE}
            String(EncodeBase64(@(self.value[1]), length(self.value))),
{$IFEND}
            #13#10,
            '',
            [rfreplaceAll]
          )
      );
end;

{ AsnUtcTime }

//constructor AsnUtcTime.Create(const xValue: TDateTime);
//begin
//  inherited;
//  FUTCOffset := 'Z';
//end;

{ AsnSystemTime }

//constructor AsnSystemTime.Create(const xValue: TDateTime);
//var
//  ts: TTimeSpan;
////  tmpHour, tmpMinutes: String;
//begin
//  inherited;
//
//  ts := TTimeZone.Local.GetUtcOffset(xValue);
//  if (ts.Minutes = 0) and (ts.Hours = 0) then
//    FUTCOffset := 'Z'
//  else
//  begin
//    if (ts.Hours < 0) or (ts.Minutes < 0) then
//      FUTCOffset := '-'
//    else
//      FUTCOffset := '+';
//
//    FUtcOffset := FUtcOffset + Format('%.*d:%.*d',[2, abs(ts.Hours), 2, abs(ts.Minutes)]);
//
////    IntToStr(abs(ts.Hours)) + ':' + IntToStr(abs(ts.Minutes));
//
//  end;
//end;

{ AnsNull }

function AsnNull.JDec(const jValue: TJSONValue): Boolean;
begin
  result := true;
end;

procedure AsnNull.JEnc(var jValue: TJSONValue);
begin
//  jValue := TJSONNull.Create();
  // estos uses 0 instead of "null"
  jValue := TJSONNumber.Create(0);
end;

{ TAsnBase }

constructor TAsnBase.Create;
begin
  inherited;
end;

end.
