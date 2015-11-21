#include "Config.hpp"
#ifdef ENABLE_OPUS

#include "OpusDecoder.hpp"
#include <opus.h>

// Opus�̎d�l
constexpr int channelNum   = 2;
constexpr int samplingRate = 48000;


using namespace std;

using sample_t = IDecoder::sample_t;
using byte_t = IDecoder::byte_t;


// �R���X�g���N�^
OpusDecoder::OpusDecoder(unique_ptr<istream> p_input, bool p_useFloat, bool p_dither) : input(move(p_input)) {
	// �����o�ݒ�
	useFloat = p_useFloat;

	// �o�b�t�@�^�C�v����
	bufferType = useFloat ? BUFFERTYPE::BUFFERTYPE_FLOAT : BUFFERTYPE::BUFFERTYPE_INT16;

	// ��O�ݒ�
	input->exceptions(istream::failbit | istream::badbit);

	// Opus���J��
	OpusFileCallbacks cb = {read, seek, tell, close};
	of = op_open_callbacks(this, &cb, nullptr, 0, nullptr);
	if (of == NULL) {
		// �ǂݍ��߂Ȃ�����
		throw ERRORCODE_INVALID_FORMAT;
	}

	// ���擾�E�ݒ�
	bufferLength = op_pcm_total(of, -1);
	channelNum = ::channelNum;
	samplingRate = ::samplingRate;

	// �f�R�[�h�ݒ�
	op_set_dither_enabled(of, p_dither ? 1 : 0);
}

// �f�X�g���N�^
OpusDecoder::~OpusDecoder() {
	// Opus�̌�Еt��
	op_free(of);
}


// �ȉ��C���^�[�t�F�C�X
sample_t OpusDecoder::_Tell() {
	ogg_int64_t position = op_pcm_tell(of);
	if (position < 0) throw position;
	return position;
}

void OpusDecoder::_Seek(sample_t position) {
	int ret = op_pcm_seek(of, position);
	if (ret < 0) throw ret;
}

sample_t OpusDecoder::_Read(void *ptr, sample_t length) {
	if (useFloat) {
		// Float�^
		int read = op_read_float_stereo(of, (float *)ptr, length * channelNum);
		if (read < 0) throw read;
		return read;
	} else {
		// �����^
		int read = op_read_stereo(of, (opus_int16 *)ptr, length * channelNum);
		if (read < 0) throw read;
		return read;
	}
}


// callbacks
// ����: �ǂݎ�����o�C�g��, ���s: ����
int OpusDecoder::read(void *stream, unsigned char *ptr, int nbytes) {
	if (stream == nullptr) return -1;
	auto decoder = (OpusDecoder *)stream;
	if (!decoder->input) return -2;
	try {
		decoder->input->read((char *)ptr, nbytes);
		return decoder->input->gcount();
	} catch (...) {
		return -3;
	}
}

// ����: 0, ���s: -1
int OpusDecoder::seek(void *stream, opus_int64 offset, int whence) {
	if (stream == nullptr) return -1;
	auto decoder = (OpusDecoder *)stream;
	if (!decoder->input) return -1;
	try {
		ios_base::seekdir way;
		switch (whence) {
			case SEEK_SET: way = ios_base::beg; break;
			case SEEK_CUR: way = ios_base::cur; break;
			case SEEK_END: way = ios_base::end; break;
			default: throw 1;
		}
		if (decoder->input->eof() && decoder->input->fail()) decoder->input->clear();
		decoder->input->seekg(offset, way);
		return 0;
	} catch (...) {
		return -1;
	}
}

// ����: �ʒu�i�o�C�g�P�ʁj, ���s: ����`
// �i�K�i�ɂ͎��s���̕Ԃ�l�͒�`����Ă��Ȃ��j
opus_int64 OpusDecoder::tell(void *stream) {
	if (stream == nullptr) return -1;
	auto decoder = (OpusDecoder *)stream;
	if (!decoder->input) return -1;
	try {
		return decoder->input->tellg();
	} catch (...) {
		return -1;
	}
}

// ����: 0, ���s: EOF
int OpusDecoder::close(void *stream) {
	if (stream == nullptr) return EOF;
	auto decoder = (OpusDecoder *)stream;
	if (!decoder->input) return EOF;
	try {
		decoder->input.reset();
		return 0;
	} catch (...) {
		return EOF;
	}
}

#endif