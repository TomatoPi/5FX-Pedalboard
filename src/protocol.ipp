
namespace sfx {
namespace io {

template <typename Object>
  std::vector<std::variant<typename parser<Object>::code, Object>>
  parser<Object>::operator() (const std::vector<std::byte>& v)
{
  assert(status::Dead != state());
  
  _buffer.insert(_buffer.end(), v.cbegin(), v.cend());
  
  std::vector<std::optional<Object>> res;
  while (is_running())
  {

    
  }  

  return res;
}

template <typename Object>
  std::optional<Object>
  parser<Object>::try_parse(raw_citerator begin, raw_citerator end)
{

}

} /**< namespace io **/
} /**< namespace sfx **/